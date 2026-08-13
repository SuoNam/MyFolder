// Implicit device registration for the web console.
//
// Every /api/v1/forwards route requires X-Device-Id + X-Device-Token, and the
// server enforces X-Device-Id === sourceDeviceId when creating a forward. So a
// browser that never registers cannot send anything. We therefore register a
// deviceType: WEB device silently on login and keep it alive with a heartbeat.
//
// The web console is a SEND-ONLY source: it never accepts an incoming forward,
// and the UI filters this device out of any target list (see stores/devices).
//
// Multi-tab: identity lives in localStorage so every tab reuses the same
// deviceId/deviceToken. Only one tab runs the heartbeat, elected through a
// lease in localStorage that the holder renews and others take over if stale.

import { shallowRef } from 'vue'
import {
  registerDevice,
  heartbeat,
  setDeviceCredentials,
  toApiError,
  type ServerDevice,
} from '@/api'

const K_ID = 'mf-device-id'
const K_TOKEN = 'mf-device-token'
const K_ACCOUNT = 'mf-device-account'
const K_LEASE = 'mf-heartbeat-lease'

export const CLIENT_VERSION = '1.1.1'

const HEARTBEAT_MS = 30_000
const LEASE_MS = 45_000 // > HEARTBEAT_MS so a live holder never looks stale

/** Stable per-browser id. Kept in localStorage so re-login reuses the device. */
function loadOrCreateDeviceId(): string {
  const existing = localStorage.getItem(K_ID)
  if (existing) return existing
  const rand =
    typeof crypto.randomUUID === 'function'
      ? crypto.randomUUID()
      : Array.from(crypto.getRandomValues(new Uint8Array(16)))
          .map((b) => b.toString(16).padStart(2, '0'))
          .join('')
  const id = `web-${rand}`
  localStorage.setItem(K_ID, id)
  return id
}

/** Human-readable name from the UA, e.g. "Chrome / Windows". */
function describeBrowser(): { name: string; os: string } {
  const ua = navigator.userAgent
  const browser =
    /Edg\//.test(ua) ? 'Edge'
    : /OPR\//.test(ua) ? 'Opera'
    : /Firefox\//.test(ua) ? 'Firefox'
    : /Chrome\//.test(ua) ? 'Chrome'
    : /Safari\//.test(ua) ? 'Safari'
    : 'Browser'
  const os =
    /Windows NT/.test(ua) ? 'Windows'
    : /Mac OS X/.test(ua) ? 'macOS'
    : /Android/.test(ua) ? 'Android'
    : /(iPhone|iPad)/.test(ua) ? 'iOS'
    : /Linux/.test(ua) ? 'Linux'
    : 'Unknown'
  return { name: `${browser} 控制台`, os: `${browser} / ${os}` }
}

export interface DeviceIdentity {
  deviceId: string
  deviceToken: string
  device: ServerDevice | null
}

// Reactive: stores keep computeds that filter on this device's id, so a plain
// variable would leave them stale after registration completes.
const identityRef = shallowRef<DeviceIdentity | null>(null)
let heartbeatTimer: number | null = null
let leaseTimer: number | null = null

export function currentIdentity(): DeviceIdentity | null {
  return identityRef.value
}

/** True for the device row representing this browser — the UI hides it. */
export function isSelf(deviceId: string): boolean {
  const id = identityRef.value
  return !!id && id.deviceId === deviceId
}

/**
 * Register (or re-register) this browser as a WEB device and start the
 * heartbeat. Safe to call repeatedly: registration is idempotent server-side
 * for a known deviceId, and it re-issues a deviceToken each time.
 */
export async function ensureRegistered(account: string): Promise<DeviceIdentity> {
  const deviceId = loadOrCreateDeviceId()

  // A cached token is only valid for the account that obtained it.
  const cachedToken = localStorage.getItem(K_TOKEN)
  const cachedAccount = localStorage.getItem(K_ACCOUNT)
  if (identityRef.value && cachedAccount === account) return identityRef.value
  if (cachedToken && cachedAccount === account) {
    identityRef.value = { deviceId, deviceToken: cachedToken, device: null }
    setDeviceCredentials(deviceId, cachedToken)
    startHeartbeat()
    // Re-register in the background so lastSeenAt/online are refreshed and a
    // token revoked server-side gets replaced. Failure here is not fatal: the
    // cached token may still be good, and forwards will surface a real error.
    void register(deviceId, account).catch(() => {})
    return identityRef.value
  }

  return register(deviceId, account)
}

async function register(deviceId: string, account: string): Promise<DeviceIdentity> {
  const { name, os } = describeBrowser()
  try {
    const { device, deviceToken } = await registerDevice({
      deviceId,
      deviceName: name,
      deviceType: 'WEB',
      os,
      clientVersion: CLIENT_VERSION,
    })
    identityRef.value = { deviceId, deviceToken, device }
    localStorage.setItem(K_TOKEN, deviceToken)
    localStorage.setItem(K_ACCOUNT, account)
    setDeviceCredentials(deviceId, deviceToken)
    startHeartbeat()
    return identityRef.value
  } catch (e) {
    throw toApiError(e)
  }
}

/** Drop local device credentials. Called on logout. */
export function forgetIdentity() {
  stopHeartbeat()
  identityRef.value = null
  localStorage.removeItem(K_TOKEN)
  localStorage.removeItem(K_ACCOUNT)
  releaseLease()
  // K_ID is intentionally kept: the same browser should stay the same device.
}

// --- heartbeat + single-tab lease ------------------------------------------

type Lease = { owner: string; expires: number }

const tabId =
  typeof crypto.randomUUID === 'function'
    ? crypto.randomUUID()
    : String(Math.random()).slice(2)

function readLease(): Lease | null {
  try {
    const raw = localStorage.getItem(K_LEASE)
    return raw ? (JSON.parse(raw) as Lease) : null
  } catch {
    return null
  }
}

/** Take the lease if it is free, expired, or already ours. */
function tryAcquireLease(): boolean {
  const now = Date.now()
  const lease = readLease()
  if (lease && lease.owner !== tabId && lease.expires > now) return false
  localStorage.setItem(K_LEASE, JSON.stringify({ owner: tabId, expires: now + LEASE_MS }))
  return true
}

function releaseLease() {
  const lease = readLease()
  if (lease?.owner === tabId) localStorage.removeItem(K_LEASE)
}

function startHeartbeat() {
  if (heartbeatTimer !== null || leaseTimer !== null) return

  const beat = async () => {
    const id = identityRef.value
    if (!id) return
    if (!tryAcquireLease()) return // another tab is the heartbeat owner
    try {
      await heartbeat(id.deviceId, CLIENT_VERSION)
    } catch (e) {
      // 404 means the server forgot this device (restart, eviction): re-register
      // so the console keeps working without a page reload.
      const err = toApiError(e)
      if (err.status === 404) {
        const account = localStorage.getItem(K_ACCOUNT)
        if (account) await register(id.deviceId, account).catch(() => {})
      }
    }
  }

  void beat()
  heartbeatTimer = window.setInterval(beat, HEARTBEAT_MS)

  // Renew the lease more often than it expires so ownership is not lost while
  // this tab is alive; a closed tab lets it lapse and another tab takes over.
  leaseTimer = window.setInterval(() => {
    const lease = readLease()
    if (lease?.owner === tabId) {
      localStorage.setItem(K_LEASE, JSON.stringify({ owner: tabId, expires: Date.now() + LEASE_MS }))
    }
  }, LEASE_MS / 3)

  window.addEventListener('beforeunload', releaseLease)
}

function stopHeartbeat() {
  if (heartbeatTimer !== null) { clearInterval(heartbeatTimer); heartbeatTimer = null }
  if (leaseTimer !== null) { clearInterval(leaseTimer); leaseTimer = null }
}
