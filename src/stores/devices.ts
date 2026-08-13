import { computed, ref } from 'vue'
import { defineStore } from 'pinia'
import { listDevices, toApiError, type ServerDevice } from '@/api'
import { isSelf } from '@/lib/device'

const POLL_MS = 10_000

/**
 * Live device list from GET /api/v1/devices.
 *
 * This browser registers itself as a WEB device so it can create forwards, but
 * it is never a valid *target* (the web console cannot receive). Every WEB
 * registration is excluded from peer/target cards, including stale records
 * left by another browser profile or an older localStorage identity.
 */
export const useDevicesStore = defineStore('devices', () => {
  const all = ref<ServerDevice[]>([])
  const loading = ref(false)
  const error = ref('')
  const loadedOnce = ref(false)

  let timer: number | null = null

  const self = computed(() => all.value.find((d) => isSelf(d.deviceId)) ?? null)
  /** Real receiving clients only, online first, then by name. */
  const peers = computed(() =>
    all.value
      .filter((d) => !isSelf(d.deviceId) && d.deviceType.toUpperCase() !== 'WEB')
      .slice()
      .sort((a, b) => {
        if (a.online !== b.online) return a.online ? -1 : 1
        return a.deviceName.localeCompare(b.deviceName, 'zh-Hans-CN')
      }),
  )
  /** Valid send targets: real peers that are currently reachable. */
  const targets = computed(() => peers.value.filter((d) => d.online))
  const onlineCount = computed(() => peers.value.filter((d) => d.online).length)

  function byId(deviceId: string) {
    return all.value.find((d) => d.deviceId === deviceId) ?? null
  }

  async function refresh() {
    loading.value = true
    try {
      all.value = await listDevices()
      error.value = ''
    } catch (e) {
      error.value = toApiError(e).message
    } finally {
      loading.value = false
      loadedOnce.value = true
    }
  }

  function startPolling() {
    if (timer !== null) return
    void refresh()
    timer = window.setInterval(() => {
      if (document.visibilityState === 'visible') void refresh()
    }, POLL_MS)
  }

  function stopPolling() {
    if (timer !== null) {
      clearInterval(timer)
      timer = null
    }
  }

  function reset() {
    stopPolling()
    all.value = []
    loadedOnce.value = false
    error.value = ''
  }

  return {
    all,
    self,
    peers,
    targets,
    onlineCount,
    loading,
    loadedOnce,
    error,
    byId,
    refresh,
    startPolling,
    stopPolling,
    reset,
  }
})
