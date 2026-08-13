// Chunked upload pipeline.
//
// Mirrors the order the desktop client uses, because the server enforces it:
//
//   1. hash every file (whole-file SHA-256, streamed — files can be GBs)
//   2. POST /file/uploadfile                   -> uploadId, chunkSize
//   3. PUT  .../chunks/{n} for every chunk     -> per-chunk SHA-256 verified
//   4. POST .../files/complete  per file       -> whole-file SHA verified, staged
//   5. POST .../complete                       -> task sealed
//   6. (send flow only) POST /api/v1/forwards  -> RELAY offer to the target
//
// Step 4 is not optional. `missingChunks: []` only means every part arrived;
// the file stays UPLOADING until /files/complete verifies content, and /complete
// answers 409 FILE_INCOMPLETE until then.
//
// Resume: the server is the source of truth. On resume we GET the task and only
// send chunks listed in missingChunks, so an interrupted upload never restarts
// from zero.

import {
  createUploadTask,
  getUploadTask,
  putChunk,
  completeUploadFile,
  completeUploadTask,
  cancelUploadTask,
  DEFAULT_CHUNK_SIZE,
  toApiError,
  type UploadTask,
} from '@/api'
import { hashBlob, hashChunk } from './sha256'

/** Parallel chunk PUTs. Enough to fill a link, few enough to stay orderly. */
const CHUNK_CONCURRENCY = 3
const MAX_CHUNK_ATTEMPTS = 3

export type UploadPhase = 'hashing' | 'creating' | 'uploading' | 'verifying' | 'done' | 'failed' | 'cancelled'

export interface UploadFileInput {
  /** Relative path inside the batch, e.g. "docs/spec.pdf". Forward slashes only. */
  path: string
  blob: Blob
}

export interface UploadProgress {
  phase: UploadPhase
  /** 0..1 across the whole batch. */
  fraction: number
  bytesSent: number
  totalBytes: number
  /** Bytes per second, smoothed. 0 until enough samples exist. */
  rate: number
  /** Seconds remaining, or null when not yet estimable. */
  etaSeconds: number | null
  currentFile: string
  error?: string
}

export interface UploadHandle {
  /** Server task id. Empty until the task is created. */
  uploadId(): string
  cancel(): void
  /** Resolves with the sealed task, or rejects on failure/cancel. */
  done: Promise<UploadTask>
}

/** Rate/ETA over a trailing window so a brief stall doesn't wreck the estimate. */
class RateMeter {
  private samples: { t: number; bytes: number }[] = []
  private readonly windowMs = 5_000

  push(totalBytesSoFar: number) {
    const now = Date.now()
    this.samples.push({ t: now, bytes: totalBytesSoFar })
    while (this.samples.length > 2 && now - this.samples[0].t > this.windowMs) {
      this.samples.shift()
    }
  }

  rate(): number {
    if (this.samples.length < 2) return 0
    const first = this.samples[0]
    const last = this.samples[this.samples.length - 1]
    const dt = (last.t - first.t) / 1000
    if (dt <= 0) return 0
    return Math.max(0, (last.bytes - first.bytes) / dt)
  }
}

/**
 * Upload a batch of files. The server writes each file directly to
 * `targetPath/<file.path>`; an empty targetPath means the storage root.
 */
export function startUpload(args: {
  files: UploadFileInput[]
  targetPath: string
  chunkSize?: number
  onProgress?: (p: UploadProgress) => void
  /** Resume an existing task instead of creating one. */
  resumeUploadId?: string
}): UploadHandle {
  const controller = new AbortController()
  let cancelled = false
  let currentUploadId = args.resumeUploadId ?? ''

  const totalBytes = args.files.reduce((sum, f) => sum + f.blob.size, 0)
  const meter = new RateMeter()

  let bytesSent = 0
  let phase: UploadPhase = 'hashing'
  let currentFile = args.files[0]?.path ?? ''

  const emit = (extra?: Partial<UploadProgress>) => {
    const rate = meter.rate()
    const remaining = Math.max(0, totalBytes - bytesSent)
    args.onProgress?.({
      phase,
      fraction: totalBytes === 0 ? 1 : Math.min(1, bytesSent / totalBytes),
      bytesSent,
      totalBytes,
      rate,
      etaSeconds: rate > 0 ? remaining / rate : null,
      currentFile,
      ...extra,
    })
  }

  const done = (async (): Promise<UploadTask> => {
    try {
      const chunkSize = args.chunkSize ?? DEFAULT_CHUNK_SIZE

      // --- 1. hash ---------------------------------------------------------
      phase = 'hashing'
      emit()
      const hashed: { path: string; blob: Blob; sha256: string; totalChunks: number }[] = []
      for (const f of args.files) {
        if (cancelled) throw new CancelledError()
        currentFile = f.path
        const sha256 = await hashBlob(f.blob, () => emit())
        hashed.push({
          path: f.path,
          blob: f.blob,
          sha256,
          totalChunks: f.blob.size === 0 ? 0 : Math.ceil(f.blob.size / chunkSize),
        })
      }

      // --- 2. create or resume --------------------------------------------
      phase = 'creating'
      emit()
      let task: UploadTask
      if (currentUploadId) {
        task = await getUploadTask(currentUploadId)
      } else {
        task = await createUploadTask({
          targetPath: args.targetPath,
          chunkSize,
          totalFiles: hashed.length,
          totalBytes,
          files: hashed.map((f) => ({
            path: f.path,
            size: f.blob.size,
            sha256: f.sha256,
            totalChunks: f.totalChunks,
          })),
        })
        currentUploadId = task.uploadId
      }

      // Server-assigned chunkSize wins — a resumed task keeps its original size.
      const effectiveChunk = task.chunkSize || chunkSize

      // --- 3. chunks -------------------------------------------------------
      phase = 'uploading'

      // Count already-completed bytes so a resumed task shows true progress.
      for (const serverFile of task.files) {
        const local = hashed.find((h) => h.path === serverFile.path)
        if (!local) continue
        for (const idx of serverFile.completedChunks) {
          const start = idx * effectiveChunk
          bytesSent += Math.min(effectiveChunk, local.blob.size - start)
        }
      }
      meter.push(bytesSent)
      emit()

      for (const local of hashed) {
        if (cancelled) throw new CancelledError()
        currentFile = local.path

        const serverFile = task.files.find((f) => f.path === local.path)
        if (serverFile?.state === 'COMPLETED') continue

        // Trust the server's view of what is missing; fall back to all chunks.
        const missing = serverFile
          ? [...serverFile.missingChunks]
          : Array.from({ length: local.totalChunks }, (_, i) => i)

        await runPool(missing, CHUNK_CONCURRENCY, async (chunkIndex) => {
          if (cancelled) throw new CancelledError()
          const start = chunkIndex * effectiveChunk
          const end = Math.min(local.blob.size, start + effectiveChunk)
          const slice = local.blob.slice(start, end)
          const buf = await slice.arrayBuffer()
          const chunkSha = await hashChunk(buf)

          let lastErr: unknown
          for (let attempt = 1; attempt <= MAX_CHUNK_ATTEMPTS; attempt++) {
            if (cancelled) throw new CancelledError()
            try {
              await putChunk({
                uploadId: currentUploadId,
                chunkIndex,
                filePath: local.path,
                chunkSha256: chunkSha,
                rangeStart: start,
                rangeEnd: end - 1,
                fileSize: local.blob.size,
                body: buf,
                signal: controller.signal,
              })
              bytesSent += end - start
              meter.push(bytesSent)
              emit()
              return
            } catch (e) {
              lastErr = e
              const err = toApiError(e)
              // 4xx other than 408/429 means the request itself is wrong —
              // retrying identical bytes will not help.
              const retryable =
                err.status === 0 || err.status === 408 || err.status === 429 || err.status >= 500
              if (!retryable || attempt === MAX_CHUNK_ATTEMPTS) throw err
              await sleep(300 * attempt)
            }
          }
          throw toApiError(lastErr)
        })

        // --- 4. per-file finalise (mandatory) ------------------------------
        phase = 'verifying'
        emit()
        task = await completeUploadFile(currentUploadId, local.path)
        phase = 'uploading'
      }

      // --- 5. seal ---------------------------------------------------------
      phase = 'verifying'
      emit()
      const sealed = await completeUploadTask(currentUploadId)
      phase = 'done'
      bytesSent = totalBytes
      emit()
      return sealed
    } catch (e) {
      if (cancelled || e instanceof CancelledError) {
        phase = 'cancelled'
        emit()
        if (currentUploadId) await cancelUploadTask(currentUploadId).catch(() => {})
        throw new CancelledError()
      }
      const err = toApiError(e)
      phase = 'failed'
      emit({ error: err.message })
      throw err
    }
  })()

  return {
    uploadId: () => currentUploadId,
    cancel: () => {
      cancelled = true
      controller.abort()
    },
    done,
  }
}

export class CancelledError extends Error {
  constructor() { super('已取消') }
}

function sleep(ms: number) {
  return new Promise((r) => setTimeout(r, ms))
}

/** Run `worker` over `items` with at most `limit` in flight. Fails fast. */
async function runPool<T>(items: T[], limit: number, worker: (item: T) => Promise<void>) {
  let cursor = 0
  const runners = Array.from({ length: Math.min(limit, items.length) }, async () => {
    while (cursor < items.length) {
      const index = cursor++
      await worker(items[index])
    }
  })
  await Promise.all(runners)
}
