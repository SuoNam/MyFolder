import axios, { type AxiosInstance } from 'axios'

// ---------------------------------------------------------------------------
// Server contract for MyFolder v1.1.1.
//
// Every endpoint and field name below was verified against the live server at
// https://api.myfolder.com.cn, not inferred. Two conventions matter:
//
//  1. Auth is the raw JWT in `Authorization` — NO "Bearer " prefix.
//  2. Two response envelopes coexist. The legacy /user/* and /file/* routes
//     wrap payloads as {status, message, data}; the /api/v1/* routes return the
//     resource directly and signal errors with an HTTP status + {code, message}.
//     `unwrap()` normalises the first kind; the second is used as-is.
// ---------------------------------------------------------------------------

export const BASE_URL =
  (import.meta.env.VITE_API_BASE as string | undefined)?.replace(/\/+$/, '') ||
  'https://api.myfolder.com.cn'

export const http: AxiosInstance = axios.create({ baseURL: BASE_URL, timeout: 30_000 })

let authToken = ''
let deviceId = ''
let deviceToken = ''
let storageScope: { type: 'PRIVATE' | 'GROUP'; id: string } = { type: 'PRIVATE', id: '' }

export function setAuth(token: string) { authToken = token }
export function setDeviceCredentials(id: string, token: string) {
  deviceId = id
  deviceToken = token
}
export function setStorageScope(type: 'PRIVATE' | 'GROUP', id = '') {
  storageScope = { type, id }
}
export function clearCredentials() {
  authToken = ''
  deviceId = ''
  deviceToken = ''
}

http.interceptors.response.use(
  (response) => response,
  (error) => {
    const status = axios.isAxiosError(error) ? error.response?.status : 0
    const path = axios.isAxiosError(error) ? String(error.config?.url ?? '') : ''
    const authRequest = path.startsWith('/user/login') || path.startsWith('/user/signup') ||
      path.startsWith('/user/refresh') || path.startsWith('/user/oauth/')
    if (status === 401 && authToken && !authRequest) {
      authToken = ''
      window.dispatchEvent(new CustomEvent('myfolder:session-expired'))
    }
    return Promise.reject(error)
  },
)

http.interceptors.request.use((config) => {
  if (authToken) config.headers.Authorization = authToken
  const path = String(config.url ?? '')
  if (path.startsWith('/file/') || path.startsWith('/directory/')) {
    if (!config.headers['X-Storage-Scope']) config.headers['X-Storage-Scope'] = storageScope.type
    if (!config.headers['X-Storage-Scope-Id'] && config.headers['X-Storage-Scope'] === 'GROUP') {
      config.headers['X-Storage-Scope-Id'] = storageScope.id
    }
  }
  return config
})

export class ApiError extends Error {
  constructor(public code: string, message: string, public status = 0) {
    super(message)
  }
}

/** Headers required by every /api/v1/forwards route. */
function deviceHeaders(): Record<string, string> {
  if (!deviceId || !deviceToken) {
    throw new ApiError('DEVICE_NOT_REGISTERED', '本机尚未注册为设备，无法操作转发任务')
  }
  return { 'X-Device-Id': deviceId, 'X-Device-Token': deviceToken }
}

type Envelope<T> = { status: number; message: string; data: T }

/** Unwrap the legacy {status,message,data} envelope, raising on a non-200 status. */
function unwrap<T>(body: Envelope<T>): T {
  if (!body || typeof body !== 'object') throw new ApiError('INVALID_RESPONSE', '服务器响应格式异常')
  if (body.status !== 200) throw new ApiError('REQUEST_FAILED', body.message || '请求失败', body.status)
  return body.data
}

/** Normalise an axios rejection into an ApiError carrying the server's own code. */
export function toApiError(e: unknown): ApiError {
  if (e instanceof ApiError) return e
  if (axios.isAxiosError(e)) {
    const status = e.response?.status ?? 0
    const body = e.response?.data as
      | { code?: string; message?: string; error?: string }
      | undefined
    if (status === 0) return new ApiError('NETWORK_ERROR', '无法连接服务器，请检查网络', 0)
    // A Spring error page carries only `error: "Internal Server Error"`, which
    // tells the user nothing. Prefer the server's own message when it has one,
    // otherwise say plainly that the failure is server-side.
    const detail = body?.message
    if (detail) return new ApiError(body?.code || `HTTP_${status}`, detail, status)
    if (status >= 500) {
      return new ApiError(body?.code || `HTTP_${status}`, `服务器处理该请求时出错（${status}）`, status)
    }
    return new ApiError(body?.code || `HTTP_${status}`, body?.error || e.message, status)
  }
  return new ApiError('UNKNOWN', e instanceof Error ? e.message : String(e))
}

// ---------------------------------------------------------------------------
// Types (field names exactly as returned by the server)
// ---------------------------------------------------------------------------

export type DeviceType = 'PC' | 'MOBILE' | 'WEB'

export interface ServerDevice {
  deviceId: string
  userId: string
  deviceName: string
  deviceType: DeviceType
  os: string
  deviceAddress: string | null
  listenPort: number | null
  clientVersion: string
  createdAt: string
  lastSeenAt: string
  online: boolean
}

export interface FileEntry {
  type: 'File' | 'Directory'
  name: string
  path: string
  size?: number
  lastModified: string
}

export type UploadState =
  | 'PENDING' | 'UPLOADING' | 'VERIFYING' | 'COMPLETED' | 'CANCELLED' | 'FAILED'

export interface UploadTaskFile {
  path: string
  size: number
  sha256: string
  totalChunks: number
  state: UploadState
  completedChunks: number[]
  missingChunks: number[]
  failureReason: string | null
}

export interface UploadTask {
  uploadId: string
  scopeType: 'PRIVATE' | 'GROUP'
  scopeId: string | null
  targetPath: string
  state: UploadState
  chunkSize: number
  totalFiles: number
  totalBytes: number
  createdAt: string
  updatedAt: string
  failureReason: string | null
  files: UploadTaskFile[]
}

export type Channel = 'AUTO' | 'LAN' | 'P2P' | 'RELAY'
export type ForwardState =
  | 'OFFERED' | 'ACCEPTED' | 'TRANSFERRING' | 'COMPLETED' | 'CANCELLED' | 'FAILED' | 'REJECTED'

export interface ForwardFile { path: string; size: number; sha256: string }

export interface Forward {
  forwardId: string
  userId: string
  sourceDeviceId: string
  targetDeviceId: string
  destinationPath: string
  deleteSource: boolean
  channel: Channel
  relayUploadId: string | null
  files: ForwardFile[]
  totalBytes: number
  transferredBytes: number
  state: ForwardState
  createdAt: string
  updatedAt: string
  failureReason: string | null
}

// ---------------------------------------------------------------------------
// Auth v2 — short-lived access JWT + rotating, revocable refresh session.
// ---------------------------------------------------------------------------

export interface AuthSession {
  token: string
  accessToken: string
  refreshToken: string
  expiresIn: number
  refreshExpiresAt: string
  account: string
  email: string | null
}

export interface OAuthProviderStatus { provider: 'nyauth' | 'google' | 'github'; configured: boolean }
export interface OAuthBinding { provider: string; username: string | null; email: string | null; boundAt: string }
export interface AccountProfile { account: string; displayName: string; email: string | null; emailVerified: boolean; autoAcceptDeviceTransfers: boolean; oauthBindings: OAuthBinding[] }

export type GroupPermission = 'READ' | 'WRITE' | 'OWNER'
export interface GroupMember { account: string; displayName: string; permission: GroupPermission; joinedAt: string }
export interface StorageGroup {
  groupId: string
  name: string
  ownerAccount: string
  myPermission: GroupPermission
  createdAt: string
  members: GroupMember[]
}
export type FolderPermission = 'NONE' | 'READ' | 'WRITE' | 'MANAGE'
export interface FolderAcl { folderPath: string; account: string; displayName: string; permission: FolderPermission }
export interface StorageUsage { tier: 'FREE' | 'PREMIUM'; systemRole: 'USER' | 'SUPER_ADMIN'; usedBytes: number; limitBytes: number | null; availableBytes: number }

export async function login(account: string, password: string): Promise<AuthSession> {
  const { data } = await http.post<Envelope<AuthSession>>('/user/login', { account, password, clientType: 'WEB' })
  return unwrap(data)
}

export async function signup(account: string, password: string, email: string, emailCode: string): Promise<AuthSession> {
  const { data } = await http.post<Envelope<AuthSession>>('/user/signup', {
    account, password, email, emailCode, clientType: 'WEB',
  })
  return unwrap(data)
}

export async function sendEmailCode(email: string, purpose: 'REGISTER' | 'RESET_PASSWORD' | 'SENSITIVE' | 'BIND_EMAIL'): Promise<string> {
  const { data } = await http.post<Envelope<{ expiresAt: string }>>('/user/email/code', { email, purpose })
  return unwrap(data).expiresAt
}

export async function resetPassword(email: string, code: string, newPassword: string): Promise<void> {
  const { data } = await http.post<Envelope<null>>('/user/password/reset', { email, code, newPassword })
  unwrap(data)
}

export async function refreshAuth(refreshToken: string): Promise<AuthSession> {
  const { data } = await http.post<Envelope<AuthSession>>('/user/refresh', { refreshToken, clientType: 'WEB' })
  return unwrap(data)
}

export async function logoutAuth(refreshToken: string): Promise<void> {
  const { data } = await http.post<Envelope<null>>('/user/logout', { refreshToken })
  unwrap(data)
}

export async function getAccountProfile(): Promise<AccountProfile> {
  const { data } = await http.get<Envelope<AccountProfile>>('/user/me')
  return unwrap(data)
}

export async function updateDisplayName(displayName: string): Promise<AccountProfile> {
  const { data } = await http.patch<Envelope<AccountProfile>>('/user/me/display-name', { displayName })
  return unwrap(data)
}

export async function updateTransferPreferences(autoAcceptDeviceTransfers: boolean): Promise<AccountProfile> {
  const { data } = await http.patch<Envelope<AccountProfile>>('/user/me/transfer-preferences', { autoAcceptDeviceTransfers })
  return unwrap(data)
}

export async function getOAuthProviders(): Promise<OAuthProviderStatus[]> {
  const { data } = await http.get<Envelope<OAuthProviderStatus[]>>('/user/oauth/providers')
  return unwrap(data)
}

export async function startOAuth(provider: string, link = false, verificationToken = ''): Promise<string> {
  const returnUri = `${window.location.origin}/oauth/callback`
  const { data } = await http.post<Envelope<{ authorizationUrl: string }>>(
    `/user/oauth/${provider}/start`, { clientType: 'WEB', returnUri, link },
    verificationToken ? { headers: { 'X-Verification-Token': verificationToken } } : undefined,
  )
  return unwrap(data).authorizationUrl
}

export async function exchangeOAuth(code: string): Promise<AuthSession> {
  const { data } = await http.post<Envelope<AuthSession>>('/user/oauth/exchange', { code, clientType: 'WEB' })
  return unwrap(data)
}

export async function verifySensitive(code: string, action: string): Promise<string> {
  const { data } = await http.post<Envelope<{ token: string }>>('/user/sensitive/verify', { code, action })
  return unwrap(data).token
}

export async function unlinkOAuth(provider: string): Promise<void> {
  const { data } = await http.delete<Envelope<null>>(`/user/oauth/${provider}`)
  unwrap(data)
}

export async function listGroups(): Promise<StorageGroup[]> {
  const { data } = await http.get<Envelope<StorageGroup[]>>('/api/v1/groups')
  return unwrap(data) ?? []
}

export async function createGroup(name: string): Promise<StorageGroup> {
  const { data } = await http.post<Envelope<StorageGroup>>('/api/v1/groups', { name })
  return unwrap(data)
}

export async function renameGroup(groupId: string, name: string): Promise<StorageGroup> {
  const { data } = await http.patch<Envelope<StorageGroup>>(`/api/v1/groups/${encodeURIComponent(groupId)}`, { name })
  return unwrap(data)
}

export async function deleteGroup(groupId: string): Promise<void> {
  const { data } = await http.delete<Envelope<null>>(`/api/v1/groups/${encodeURIComponent(groupId)}`)
  unwrap(data)
}

export async function addGroupMember(groupId: string, email: string, permission: 'READ' | 'WRITE'): Promise<StorageGroup> {
  const { data } = await http.post<Envelope<StorageGroup>>(`/api/v1/groups/${encodeURIComponent(groupId)}/members`, { email, permission })
  return unwrap(data)
}

export async function leaveGroup(groupId: string): Promise<void> {
  const { data } = await http.post<Envelope<null>>(`/api/v1/groups/${encodeURIComponent(groupId)}/leave`)
  unwrap(data)
}

export async function updateGroupMember(groupId: string, account: string, permission: 'READ' | 'WRITE'): Promise<StorageGroup> {
  const { data } = await http.patch<Envelope<StorageGroup>>(`/api/v1/groups/${encodeURIComponent(groupId)}/members/${encodeURIComponent(account)}`, { permission })
  return unwrap(data)
}

export async function removeGroupMember(groupId: string, account: string): Promise<void> {
  const { data } = await http.delete<Envelope<null>>(`/api/v1/groups/${encodeURIComponent(groupId)}/members/${encodeURIComponent(account)}`)
  unwrap(data)
}

export async function getFolderAcl(groupId: string, folderPath: string): Promise<FolderAcl[]> {
  const { data } = await http.get<Envelope<FolderAcl[]>>(`/api/v1/groups/${encodeURIComponent(groupId)}/acl`, { params: { folderPath } })
  return unwrap(data) ?? []
}
export async function setFolderAcl(groupId: string, folderPath: string, account: string, permission: FolderPermission): Promise<FolderAcl[]> {
  const { data } = await http.put<Envelope<FolderAcl[]>>(`/api/v1/groups/${encodeURIComponent(groupId)}/acl`, { folderPath, account, permission })
  return unwrap(data) ?? []
}
export async function removeFolderAcl(groupId: string, folderPath: string, account: string): Promise<void> {
  const { data } = await http.delete<Envelope<null>>(`/api/v1/groups/${encodeURIComponent(groupId)}/acl/${encodeURIComponent(account)}`, { params: { folderPath } })
  unwrap(data)
}
export async function getStorageUsage(): Promise<StorageUsage> {
  const { data } = await http.get<Envelope<StorageUsage>>('/user/me/storage')
  return unwrap(data)
}

// ---------------------------------------------------------------------------
// Files  —  legacy envelope routes
// ---------------------------------------------------------------------------

/** POST /file/getfilelist {directoryPath} — "" is the account root. */
export async function listFiles(directoryPath: string): Promise<FileEntry[]> {
  const { data } = await http.post<Envelope<FileEntry[]>>('/file/getfilelist', { directoryPath })
  return unwrap(data) ?? []
}

export async function listScopedFiles(directoryPath: string, type: 'PRIVATE' | 'GROUP', id = ''): Promise<FileEntry[]> {
  const headers: Record<string, string> = { 'X-Storage-Scope': type }
  if (type === 'GROUP') headers['X-Storage-Scope-Id'] = id
  const { data } = await http.post<Envelope<FileEntry[]>>('/file/getfilelist', { directoryPath }, { headers })
  return unwrap(data) ?? []
}

export async function movePrivatePathsToGroup(sourcePaths: string[], groupId: string, targetDirectory: string): Promise<void> {
  const { data } = await http.post<Envelope<Envelope<string>[]>>('/file/move-to-group', {
    sourcePaths, groupId, targetDirectory,
  })
  unwrap(data)
}

export async function copyPrivatePathsToGroup(sourcePaths: string[], groupId: string, targetDirectory: string): Promise<void> {
  const { data } = await http.post<Envelope<Envelope<string>[]>>('/file/copy-to-group', {
    groupId,
    targetDirectory,
    sourcePaths,
  })
  unwrap(data)
}

/** POST /file/createfolder {path} — the key is `path`, NOT `directoryPath`. */
export async function createFolder(path: string): Promise<void> {
  const { data } = await http.post<Envelope<null>>('/file/createfolder', { path })
  unwrap(data)
}

/**
 * POST /file/move [{targetPath,newPath}] — rename or relocate.
 *
 * The server's field names are misleading: `targetPath` is the SOURCE and
 * `newPath` is the destination. This wrapper takes (from, to) instead.
 *
 * Verified against the deployed server:
 *   - rename within a directory            -> 200
 *   - destination already exists           -> 200 and the destination is
 *     SILENTLY OVERWRITTEN (Files.move REPLACE_EXISTING). Callers must check
 *     for a collision first; the server will not stop you losing data.
 *   - destination's parent does not exist  -> 404 FILE_NOT_FOUND (no mkdir -p)
 *   - source does not exist                -> 404 FILE_NOT_FOUND
 *   - destination outside the root          -> 400 INVALID_PATH
 *
 * Not atomic: a batch aborts on the first failure without rolling back.
 */
export async function movePaths(
  moves: { from: string; to: string }[],
): Promise<{ path: string; ok: boolean; message: string }[]> {
  const body = moves.map((m) => ({ targetPath: m.from, newPath: m.to }))
  const { data } = await http.post<Envelope<Envelope<string>[]>>('/file/move', body)
  const results = unwrap(data) ?? []
  return results.map((r) => ({ path: r.data, ok: r.status === 200, message: r.message }))
}

/**
 * POST /file/copy [{targetPath,newPath}] — copy a file or a directory tree.
 *
 * Same (from, to) normalisation and the same overwrite hazard as movePaths.
 * Additionally: copying a directory into itself -> 400 INVALID_TARGET.
 */
export async function copyPaths(
  copies: { from: string; to: string }[],
): Promise<{ path: string; ok: boolean; message: string }[]> {
  const body = copies.map((c) => ({ targetPath: c.from, newPath: c.to }))
  const { data } = await http.post<Envelope<Envelope<string>[]>>('/file/copy', body)
  const results = unwrap(data) ?? []
  return results.map((r) => ({ path: r.data, ok: r.status === 200, message: r.message }))
}

/**
 * POST /file/delete [{deletePath}] — body is an ARRAY.
 *
 * Verified against the deployed server: files and directories (recursively) both
 * return 200 with a per-item envelope; a missing path is 404 FILE_NOT_FOUND and
 * a path outside the root is 400 INVALID_PATH.
 *
 * Not atomic: the server deletes items in order and aborts on the first failure,
 * so earlier items stay deleted while the response is an error. The caller must
 * re-list rather than assume nothing happened.
 */
export interface DeleteResult {
  path: string
  ok: boolean
  message: string
}

export async function deletePaths(paths: string[]): Promise<DeleteResult[]> {
  const body = paths.map((p) => ({ deletePath: p }))
  const { data } = await http.post<Envelope<Envelope<string>[]>>('/file/delete', body)
  const results = unwrap(data) ?? []
  return results.map((r) => ({ path: r.data, ok: r.status === 200, message: r.message }))
}

/** Prefer RFC 5987 filename*, fall back to the quoted form (skip RFC 2047 blobs). */
function filenameFrom(disposition: string): string {
  const star = /filename\*=UTF-8''([^;]+)/i.exec(disposition)
  if (star) { try { return decodeURIComponent(star[1]) } catch { /* fall through */ } }
  const plain = /filename="?([^";]+)"?/i.exec(disposition)
  if (plain && !plain[1].startsWith('=?')) return plain[1]
  return ''
}

/** Authenticated download. Blob-based so the JWT header can be attached. */
export async function downloadFile(
  filePath: string,
  onProgress?: (loaded: number, total: number) => void,
): Promise<{ blob: Blob; filename: string }> {
  const res = await http.get<Blob>('/file/downloadfile', {
    params: { filePathS: filePath },
    responseType: 'blob',
    onDownloadProgress: (e) => onProgress?.(e.loaded, e.total ?? 0),
  })
  const disposition = String(res.headers['content-disposition'] ?? '')
  return {
    blob: res.data,
    filename: filenameFrom(disposition) || filePath.split('/').pop() || 'download',
  }
}

/**
 * GET /directory/downloaddirectory?directoryPathS=… — the folder as a ZIP.
 *
 * Verified against the deployed server: returns a real ZIP stream (PK magic),
 * `Content-Disposition: attachment; filename="<name>.zip"`, and the response
 * does expose Content-Disposition via Access-Control-Expose-Headers.
 *
 * The name is percent-encoded with Java's URLEncoder rather than RFC 5987, so
 * spaces arrive as '+' and non-ASCII as %XX. Decode it here; fall back to the
 * path's last segment if anything about the header looks wrong.
 */
export async function downloadDirectory(
  directoryPath: string,
  onProgress?: (loaded: number, total: number) => void,
): Promise<{ blob: Blob; filename: string }> {
  const res = await http.get<Blob>('/directory/downloaddirectory', {
    params: { directoryPathS: directoryPath },
    responseType: 'blob',
    onDownloadProgress: (e) => onProgress?.(e.loaded, e.total ?? 0),
  })
  const raw = filenameFrom(String(res.headers['content-disposition'] ?? ''))
  let name = ''
  if (raw) {
    try {
      name = decodeURIComponent(raw.replace(/\+/g, ' '))
    } catch {
      name = raw
    }
  }
  const fallback = `${directoryPath.split('/').filter(Boolean).pop() || 'files'}.zip`
  return { blob: res.data, filename: name || fallback }
}

// ---------------------------------------------------------------------------
// Devices  —  /api/v1/devices
// ---------------------------------------------------------------------------

export async function listDevices(): Promise<ServerDevice[]> {
  const { data } = await http.get<ServerDevice[]>('/api/v1/devices')
  return Array.isArray(data) ? data : []
}

export async function registerDevice(body: {
  deviceId: string
  deviceName: string
  deviceType: DeviceType
  os: string
  clientVersion: string
  deviceAddress?: string
}): Promise<{ device: ServerDevice; deviceToken: string }> {
  const { data } = await http.post<{ device: ServerDevice; deviceToken: string }>(
    '/api/v1/devices', body,
  )
  return data
}

export async function heartbeat(id: string, clientVersion: string): Promise<void> {
  await http.post(`/api/v1/devices/${encodeURIComponent(id)}/heartbeat`, { clientVersion })
}

export async function unregisterDevice(id: string): Promise<void> {
  await http.delete(`/api/v1/devices/${encodeURIComponent(id)}`)
}

// ---------------------------------------------------------------------------
// Upload  —  /file/uploadfile
//
// Finalisation is a two-stage handshake and BOTH stages are mandatory:
//   PUT all chunks -> POST /files/complete (per file: verifies the whole-file
//   SHA-256 and moves it out of staging) -> POST /complete (seals the task).
// Skipping /files/complete makes /complete return 409 FILE_INCOMPLETE even when
// missingChunks is empty: empty chunks means "all parts arrived", not "verified".
// ---------------------------------------------------------------------------

export const DEFAULT_CHUNK_SIZE = 4 * 1024 * 1024
const UPLOAD_TASKS_PATH = '/file/uploadfile'

export async function createUploadTask(body: {
  targetPath: string
  chunkSize: number
  totalFiles: number
  totalBytes: number
  files: { path: string; size: number; sha256: string; totalChunks: number }[]
}): Promise<UploadTask> {
  const { data } = await http.post<UploadTask>(UPLOAD_TASKS_PATH, {
    ...body,
    scopeType: storageScope.type,
    scopeId: storageScope.type === 'GROUP' ? storageScope.id : null,
  })
  return data
}

export async function getUploadTask(uploadId: string): Promise<UploadTask> {
  const { data } = await http.get<UploadTask>(
    `${UPLOAD_TASKS_PATH}/${encodeURIComponent(uploadId)}`,
  )
  return data
}

export async function putChunk(args: {
  uploadId: string
  chunkIndex: number
  filePath: string
  chunkSha256: string
  rangeStart: number
  rangeEnd: number
  fileSize: number
  body: ArrayBuffer
  signal?: AbortSignal
  onProgress?: (loaded: number) => void
}): Promise<UploadTask> {
  const { data } = await http.put<UploadTask>(
    `${UPLOAD_TASKS_PATH}/${encodeURIComponent(args.uploadId)}/chunks/${args.chunkIndex}`,
    args.body,
    {
      headers: {
        'Content-Type': 'application/octet-stream',
        'X-File-Path': encodeURIComponent(args.filePath),
        'X-Chunk-SHA256': args.chunkSha256,
        'Content-Range': `bytes ${args.rangeStart}-${args.rangeEnd}/${args.fileSize}`,
      },
      signal: args.signal,
      onUploadProgress: (e) => args.onProgress?.(e.loaded),
    },
  )
  return data
}

export async function completeUploadFile(uploadId: string, filePath: string): Promise<UploadTask> {
  const { data } = await http.post<UploadTask>(
    `${UPLOAD_TASKS_PATH}/${encodeURIComponent(uploadId)}/files/complete`,
    { filePath },
  )
  return data
}

export async function completeUploadTask(uploadId: string): Promise<UploadTask> {
  const { data } = await http.post<UploadTask>(
    `${UPLOAD_TASKS_PATH}/${encodeURIComponent(uploadId)}/complete`, {},
  )
  return data
}

export async function cancelUploadTask(uploadId: string): Promise<UploadTask> {
  const { data } = await http.delete<UploadTask>(
    `${UPLOAD_TASKS_PATH}/${encodeURIComponent(uploadId)}`,
  )
  return data
}

// ---------------------------------------------------------------------------
// Forwards  —  /api/v1/forwards (all require the device headers)
//
// The server enforces X-Device-Id === sourceDeviceId on create, which is why a
// web client that never registers cannot create or list its own forwards.
// accept/start/progress/complete are receiver-side actions and return 404 for
// the source device, so they are deliberately not exposed here.
// ---------------------------------------------------------------------------

export async function listForwards(): Promise<Forward[]> {
  const { data } = await http.get<Forward[]>('/api/v1/forwards', { headers: deviceHeaders() })
  return Array.isArray(data) ? data : []
}

/** Account-wide ledger, including transfers performed by the user's other devices. */
export async function listForwardHistory(): Promise<Forward[]> {
  const { data } = await http.get<Forward[]>('/api/v1/forwards/history', { headers: deviceHeaders() })
  return Array.isArray(data) ? data : []
}

/** Account-wide resumable uploads, used as the authoritative upload ledger. */
export async function listUploadTasks(): Promise<UploadTask[]> {
  const { data } = await http.get<UploadTask[]>('/api/v1/transfers/tasks')
  return Array.isArray(data) ? data : []
}

export async function getForward(forwardId: string): Promise<Forward> {
  const { data } = await http.get<Forward>(
    `/api/v1/forwards/${encodeURIComponent(forwardId)}`, { headers: deviceHeaders() },
  )
  return data
}

export async function createRelayForward(args: {
  sourceDeviceId: string
  targetDeviceId: string
  destinationPath: string
  relayUploadId: string
  files: ForwardFile[]
}): Promise<Forward> {
  const { data } = await http.post<Forward>(
    '/api/v1/forwards',
    {
      sourceDeviceId: args.sourceDeviceId,
      targetDeviceId: args.targetDeviceId,
      destinationPath: args.destinationPath,
      deleteSource: false,
      channel: 'RELAY',
      relayUploadId: args.relayUploadId,
      files: args.files,
    },
    { headers: deviceHeaders() },
  )
  return data
}

export async function cancelForward(forwardId: string): Promise<Forward> {
  const { data } = await http.delete<Forward>(
    `/api/v1/forwards/${encodeURIComponent(forwardId)}`, { headers: deviceHeaders() },
  )
  return data
}
