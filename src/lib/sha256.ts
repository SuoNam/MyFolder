// Incremental SHA-256.
//
// Why not crypto.subtle.digest? It is one-shot: the whole file must be in memory
// as a single buffer. MyFolder sends multi-GB directories, so the per-file hash
// has to be computed by streaming slices. crypto.subtle IS used for the per-chunk
// hash (a chunk is bounded at chunkSize and already in memory).

const K = new Uint32Array([
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
  0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
  0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
  0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
  0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
  0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
])

export class Sha256 {
  private h = new Uint32Array([
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
  ])
  private buf = new Uint8Array(64)
  private bufLen = 0
  private total = 0
  private w = new Uint32Array(64)

  update(data: Uint8Array): void {
    this.total += data.length
    let offset = 0

    if (this.bufLen > 0) {
      const need = 64 - this.bufLen
      const take = Math.min(need, data.length)
      this.buf.set(data.subarray(0, take), this.bufLen)
      this.bufLen += take
      offset = take
      if (this.bufLen < 64) return
      this.block(this.buf, 0)
      this.bufLen = 0
    }

    while (offset + 64 <= data.length) {
      this.block(data, offset)
      offset += 64
    }

    if (offset < data.length) {
      this.buf.set(data.subarray(offset), 0)
      this.bufLen = data.length - offset
    }
  }

  digestHex(): string {
    // Pad: 0x80, zeros, then the 64-bit big-endian bit length.
    const bitLen = this.total * 8
    const padLen = this.bufLen < 56 ? 56 - this.bufLen : 120 - this.bufLen
    const tail = new Uint8Array(padLen + 8)
    tail[0] = 0x80
    // Bit length as two 32-bit halves keeps this exact past 2^32 bits (512 MB).
    const hi = Math.floor(bitLen / 0x100000000)
    const lo = bitLen >>> 0
    const dv = new DataView(tail.buffer)
    dv.setUint32(padLen, hi)
    dv.setUint32(padLen + 4, lo)
    this.update(tail)

    let out = ''
    for (let i = 0; i < 8; i++) out += this.h[i].toString(16).padStart(8, '0')
    return out
  }

  private block(p: Uint8Array, off: number): void {
    const w = this.w
    for (let i = 0; i < 16; i++) {
      const j = off + i * 4
      w[i] = ((p[j] << 24) | (p[j + 1] << 16) | (p[j + 2] << 8) | p[j + 3]) >>> 0
    }
    for (let i = 16; i < 64; i++) {
      const x = w[i - 15]
      const y = w[i - 2]
      const s0 = ((x >>> 7) | (x << 25)) ^ ((x >>> 18) | (x << 14)) ^ (x >>> 3)
      const s1 = ((y >>> 17) | (y << 15)) ^ ((y >>> 19) | (y << 13)) ^ (y >>> 10)
      w[i] = (w[i - 16] + s0 + w[i - 7] + s1) >>> 0
    }

    let [a, b, c, d, e, f, g, h] = this.h

    for (let i = 0; i < 64; i++) {
      const S1 = ((e >>> 6) | (e << 26)) ^ ((e >>> 11) | (e << 21)) ^ ((e >>> 25) | (e << 7))
      const ch = (e & f) ^ (~e & g)
      const t1 = (h + S1 + ch + K[i] + w[i]) >>> 0
      const S0 = ((a >>> 2) | (a << 30)) ^ ((a >>> 13) | (a << 19)) ^ ((a >>> 22) | (a << 10))
      const maj = (a & b) ^ (a & c) ^ (b & c)
      const t2 = (S0 + maj) >>> 0
      h = g; g = f; f = e
      e = (d + t1) >>> 0
      d = c; c = b; b = a
      a = (t1 + t2) >>> 0
    }

    const hh = this.h
    hh[0] = (hh[0] + a) >>> 0
    hh[1] = (hh[1] + b) >>> 0
    hh[2] = (hh[2] + c) >>> 0
    hh[3] = (hh[3] + d) >>> 0
    hh[4] = (hh[4] + e) >>> 0
    hh[5] = (hh[5] + f) >>> 0
    hh[6] = (hh[6] + g) >>> 0
    hh[7] = (hh[7] + h) >>> 0
  }
}

/** Hash a whole File/Blob by streaming 4 MB slices. Never buffers the file. */
export async function hashBlob(
  blob: Blob,
  onProgress?: (done: number, total: number) => void,
): Promise<string> {
  const SLICE = 4 * 1024 * 1024
  const h = new Sha256()
  let offset = 0
  while (offset < blob.size) {
    const end = Math.min(blob.size, offset + SLICE)
    const part = new Uint8Array(await blob.slice(offset, end).arrayBuffer())
    h.update(part)
    offset = end
    onProgress?.(offset, blob.size)
  }
  return h.digestHex()
}

/**
 * Per-chunk hash. Use the same incremental implementation as whole files.
 * `crypto.subtle` is unavailable when a user reaches the console over plain
 * HTTP; that used to leave a server task in PENDING immediately after create.
 */
export async function hashChunk(data: ArrayBuffer): Promise<string> {
  const hash = new Sha256()
  hash.update(new Uint8Array(data))
  return hash.digestHex()
}
