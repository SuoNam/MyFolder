import { computed, ref } from 'vue'
import { defineStore } from 'pinia'
import {
  cancelForward,
  createRelayForward,
  listForwards,
  toApiError,
  type Forward,
  type ForwardFile,
} from '@/api'
import { currentIdentity } from '@/lib/device'
import { startUpload, type UploadFileInput, type UploadHandle, type UploadPhase } from '@/lib/upload'
import { joinPath } from '@/lib/format'

const JOBS_KEY = 'mf-jobs'
const POLL_MS = 4_000
const KEEP_JOBS = 60

/**
 * A local job record. The server has no "list my upload tasks" endpoint, so the
 * console keeps its own ledger in localStorage: without it, a page reload would
 * lose every trace of what this browser sent.
 */
export interface Job {
  id: string
  /** 'send' uploads then forwards to a peer; 'upload' only stores on the server. */
  kind: 'send' | 'upload'
  title: string
  parentPath: string
  targetDeviceId: string
  targetName: string
  destinationPath: string
  uploadId: string
  forwardId: string
  fileCount: number
  totalBytes: number
  bytesSent: number
  fraction: number
  rate: number
  etaSeconds: number | null
  currentFile: string
  phase: UploadPhase
  error: string
  createdAt: string
  updatedAt: string
}

const ACTIVE_PHASES: UploadPhase[] = ['hashing', 'creating', 'uploading', 'verifying']
const isActivePhase = (p: UploadPhase) => ACTIVE_PHASES.includes(p)

/** Human label for an upload phase. Shared by every view that shows a job. */
export const PHASE_LABEL: Record<UploadPhase, string> = {
  hashing: '校验中',
  creating: '创建任务',
  uploading: '上传中',
  verifying: '服务器校验',
  done: '已完成',
  failed: '失败',
  cancelled: '已取消',
}

/** Human label for a forward state, as seen from the sending side. */
export const FORWARD_LABEL: Record<string, string> = {
  OFFERED: '等待对方接收',
  ACCEPTED: '对方已接收',
  TRANSFERRING: '对方下载中',
  COMPLETED: '已送达',
  CANCELLED: '已取消',
  FAILED: '失败',
  REJECTED: '对方已拒绝',
}

function loadJobs(): Job[] {
  try {
    const raw = localStorage.getItem(JOBS_KEY)
    if (!raw) return []
    const parsed = JSON.parse(raw) as Job[]
    if (!Array.isArray(parsed)) return []
    // Anything still "active" in storage was interrupted by a reload or a crash.
    return parsed.map((j) =>
      isActivePhase(j.phase)
        ? { ...j, phase: 'failed' as UploadPhase, error: j.error || '页面刷新中断，未完成' }
        : j,
    )
  } catch {
    return []
  }
}

export const useTransfersStore = defineStore('transfers', () => {
  const jobs = ref<Job[]>(loadJobs())
  const forwards = ref<Forward[]>([])
  const error = ref('')
  const forwardsError = ref('')
  const loadedOnce = ref(false)

  /** Live upload handles, keyed by job id. Not persisted. */
  const handles = new Map<string, UploadHandle>()
  let timer: number | null = null

  function persist() {
    try {
      localStorage.setItem(JOBS_KEY, JSON.stringify(jobs.value.slice(0, KEEP_JOBS)))
    } catch {
      /* storage full or blocked — the in-memory list still works */
    }
  }

  function patch(id: string, next: Partial<Job>) {
    const i = jobs.value.findIndex((j) => j.id === id)
    if (i === -1) return
    jobs.value[i] = { ...jobs.value[i], ...next, updatedAt: new Date().toISOString() }
    persist()
  }

  const activeJobs = computed(() => jobs.value.filter((j) => isActivePhase(j.phase)))
  const doneJobs = computed(() => jobs.value.filter((j) => !isActivePhase(j.phase)))

  /** Forwards this browser created. The server returns both directions. */
  const myForwards = computed(() => {
    const id = currentIdentity()?.deviceId
    return id ? forwards.value.filter((f) => f.sourceDeviceId === id) : []
  })

  const liveForwards = computed(() =>
    myForwards.value.filter((f) => !['COMPLETED', 'CANCELLED', 'FAILED', 'REJECTED'].includes(f.state)),
  )
  const pastForwards = computed(() =>
    myForwards.value.filter((f) => ['COMPLETED', 'CANCELLED', 'FAILED', 'REJECTED'].includes(f.state)),
  )

  const activeCount = computed(() => activeJobs.value.length + liveForwards.value.length)

  function forwardOf(job: Job): Forward | null {
    if (!job.forwardId) return null
    return forwards.value.find((f) => f.forwardId === job.forwardId) ?? null
  }

  async function refreshForwards() {
    if (!currentIdentity()) {
      forwardsError.value = '此浏览器尚未注册为设备，无法读取转发列表'
      return
    }
    try {
      forwards.value = await listForwards()
      forwardsError.value = ''
    } catch (e) {
      forwardsError.value = toApiError(e).message
    } finally {
      loadedOnce.value = true
    }
  }

  function startPolling() {
    if (timer !== null) return
    void refreshForwards()
    timer = window.setInterval(() => {
      if (document.visibilityState === 'visible') void refreshForwards()
    }, POLL_MS)
  }

  function stopPolling() {
    if (timer !== null) {
      clearInterval(timer)
      timer = null
    }
  }

  /**
   * Upload a batch and, for kind 'send', create a RELAY forward to a peer.
   *
   * Order is fixed by the server: create task -> all chunks -> per-file
   * /files/complete -> task /complete -> POST /api/v1/forwards. The forward is
   * only created after the task is sealed, because it references relayUploadId.
   */
  function enqueue(opts: {
    files: UploadFileInput[]
    title: string
    /** Server-side destination folder. Empty means the storage root. */
    targetPath?: string
    target?: { deviceId: string; deviceName: string } | null
    /** Where the peer should put the files. */
    destinationPath?: string
  }): string {
    const identity = currentIdentity()
    const kind: Job['kind'] = opts.target ? 'send' : 'upload'

    if (kind === 'send' && !identity) {
      error.value = '此浏览器尚未注册为设备，无法发送到其他设备'
      return ''
    }

    const now = new Date().toISOString()
    const id = `job-${Date.now()}-${Math.random().toString(36).slice(2, 8)}`
    const totalBytes = opts.files.reduce((s, f) => s + f.blob.size, 0)

    const targetPath = opts.targetPath?.replace(/^\/+|\/+$/g, '') ?? ''

    const job: Job = {
      id,
      kind,
      title: opts.title,
      parentPath: targetPath,
      targetDeviceId: opts.target?.deviceId ?? '',
      targetName: opts.target?.deviceName ?? '',
      destinationPath: opts.destinationPath || 'MyFolder',
      uploadId: '',
      forwardId: '',
      fileCount: opts.files.length,
      totalBytes,
      bytesSent: 0,
      fraction: 0,
      rate: 0,
      etaSeconds: null,
      currentFile: opts.files[0]?.path ?? '',
      phase: 'hashing',
      error: '',
      createdAt: now,
      updatedAt: now,
    }
    jobs.value = [job, ...jobs.value]
    persist()

    // startUpload emits its first progress event synchronously, so onProgress
    // must not close over `handle` directly — it is still in its TDZ then.
    let live: UploadHandle | null = null

    const handle = startUpload({
      files: opts.files,
      targetPath,
      onProgress: (p) => {
        const seenUploadId = live?.uploadId() ?? ''
        patch(id, {
          phase: p.phase,
          fraction: p.fraction,
          bytesSent: p.bytesSent,
          rate: p.rate,
          etaSeconds: p.etaSeconds,
          currentFile: p.currentFile,
          error: p.error ?? '',
          // Keep any id already recorded rather than clearing it.
          ...(seenUploadId ? { uploadId: seenUploadId } : {}),
        })
      },
    })
    live = handle
    handles.set(id, handle)

    void handle.done
      .then(async (task) => {
        patch(id, { uploadId: task.uploadId })
        if (kind !== 'send' || !identity) {
          patch(id, { phase: 'done', fraction: 1, bytesSent: job.totalBytes })
          return
        }
        // Files carry the hashes the server already verified during finalise.
        const files: ForwardFile[] = task.files.map((f) => ({
          path: f.path,
          size: f.size,
          sha256: f.sha256,
        }))
        try {
          const fw = await createRelayForward({
            sourceDeviceId: identity.deviceId,
            targetDeviceId: job.targetDeviceId,
            destinationPath: job.destinationPath,
            relayUploadId: task.uploadId,
            files,
          })
          patch(id, { forwardId: fw.forwardId, phase: 'done', fraction: 1 })
          await refreshForwards()
        } catch (e) {
          // The bytes are safely on the server; only the handoff failed.
          patch(id, { phase: 'failed', error: `上传成功，创建转发失败：${toApiError(e).message}` })
        }
      })
      .catch((e) => {
        const msg = e instanceof Error ? e.message : String(e)
        patch(id, { phase: msg === '已取消' ? 'cancelled' : 'failed', error: msg })
      })
      .finally(() => handles.delete(id))

    return id
  }

  function cancelJob(id: string) {
    handles.get(id)?.cancel()
    const job = jobs.value.find((j) => j.id === id)
    if (job?.forwardId) void cancelForward(job.forwardId).catch(() => {})
    if (job && !isActivePhase(job.phase)) return
    patch(id, { phase: 'cancelled', error: '已取消' })
  }

  async function cancelForwardById(forwardId: string) {
    try {
      await cancelForward(forwardId)
      await refreshForwards()
      return true
    } catch (e) {
      forwardsError.value = toApiError(e).message
      return false
    }
  }

  function removeJob(id: string) {
    const job = jobs.value.find((j) => j.id === id)
    if (job && isActivePhase(job.phase)) return
    jobs.value = jobs.value.filter((j) => j.id !== id)
    persist()
  }

  function clearFinished() {
    jobs.value = jobs.value.filter((j) => isActivePhase(j.phase))
    persist()
  }

  function reset() {
    stopPolling()
    for (const h of handles.values()) h.cancel()
    handles.clear()
    forwards.value = []
    loadedOnce.value = false
  }

  return {
    jobs,
    forwards,
    activeJobs,
    doneJobs,
    myForwards,
    liveForwards,
    pastForwards,
    activeCount,
    error,
    forwardsError,
    loadedOnce,
    forwardOf,
    enqueue,
    cancelJob,
    cancelForwardById,
    removeJob,
    clearFinished,
    refreshForwards,
    startPolling,
    stopPolling,
    reset,
    joinPath,
  }
})
