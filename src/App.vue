<template>
  <div v-if="['login', 'oauth-callback'].includes(String(route.name))"><RouterView /></div>
  <div v-else class="shell">
    <header class="mobile-appbar" :inert="navigationOpen">
      <button ref="menuToggle" class="btn mobile-menu-toggle" aria-label="打开导航菜单" aria-controls="main-navigation" :aria-expanded="navigationOpen" @click="openNavigation"><svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M4 6h16M4 12h16M4 18h16" /></svg></button>
      <img class="brand-mark" src="/myfolder-icon-v2.png" alt="" />
      <div class="mobile-appbar-title"><strong>MyFolder</strong><span>{{ pageLabel }}</span></div>
      <span class="mobile-account">{{ user.account }}</span>
    </header>
    <div v-if="isMobile && navigationOpen" class="navigation-backdrop" aria-hidden="true" @click="closeNavigation()" />
    <nav id="main-navigation" ref="navigation" class="rail" :class="{ 'is-open': navigationOpen }" :role="isMobile ? 'dialog' : undefined" :aria-modal="isMobile && navigationOpen ? true : undefined" :aria-hidden="isMobile && !navigationOpen ? true : undefined" :inert="isMobile && !navigationOpen" aria-label="主导航" @keydown="onNavigationKeydown">
      <div class="brand">
        <img class="brand-mark" src="/myfolder-icon-v2.png" alt="" />
        <div class="brand-name">MyFolder</div>
        <div class="brand-ver">{{ CLIENT_VERSION }}</div>
      </div>
      <button ref="menuClose" class="btn mobile-navigation-close" aria-label="关闭导航菜单" @click="closeNavigation()">关闭 <span aria-hidden="true">×</span></button>

      <div class="nav-group">
        <div class="nav-head eyebrow">Workspace</div>
        <div class="nav">
          <RouterLink class="nav-item" :class="{ 'is-active': route.name === 'files' }" to="/files">
            <Icon name="folder" /><span>文件</span>
          </RouterLink>
          <RouterLink class="nav-item" :class="{ 'is-active': route.name === 'groups' }" to="/groups">
            <Icon name="devices" /><span>群组</span>
          </RouterLink>
          <RouterLink class="nav-item" :class="{ 'is-active': route.name === 'devices' }" to="/devices">
            <Icon name="devices" /><span>设备</span>
            <span class="count">{{ devices.peers.length || '' }}</span>
          </RouterLink>
          <RouterLink class="nav-item" :class="{ 'is-active': route.name === 'transfers' }" to="/transfers">
            <Icon name="transfer" /><span>传输</span>
            <span class="count">{{ transfers.activeCount || '' }}</span>
          </RouterLink>
        </div>
      </div>
      <div class="nav-group">
        <div class="nav-head eyebrow">Records</div>
        <div class="nav">
          <RouterLink class="nav-item" :class="{ 'is-active': route.name === 'history' }" to="/history">
            <Icon name="history" /><span>传输记录</span>
          </RouterLink>
          <RouterLink class="nav-item" :class="{ 'is-active': route.name === 'settings' }" to="/settings">
            <Icon name="settings" /><span>设置</span>
          </RouterLink>
        </div>
      </div>

      <div class="rail-foot">
        <div class="me">
          <div class="me-av">{{ initials }}</div>
          <div>
            <div class="me-name">{{ user.account }}</div>
            <div class="me-sub">
              <span class="led" :class="user.deviceReady ? 'is-on' : ''" />
              {{ user.deviceReady ? 'Web 控制台已就绪' : '设备未注册' }}
            </div>
          </div>
        </div>
      </div>
    </nav>

    <div class="main" :inert="isMobile && navigationOpen"><RouterView /></div>
  </div>
</template>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { RouterLink, RouterView, useRoute } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { useDevicesStore } from '@/stores/devices'
import { useTransfersStore } from '@/stores/transfers'
import Icon from '@/components/Icon.vue'
import { CLIENT_VERSION } from '@/lib/device'

const route = useRoute()
const user = useUserStore()
const devices = useDevicesStore()
const transfers = useTransfersStore()
const mobileQuery = window.matchMedia('(max-width: 760px)')
const isMobile = ref(mobileQuery.matches)
const navigationOpen = ref(false)
const navigation = ref<HTMLElement | null>(null)
const menuToggle = ref<HTMLButtonElement | null>(null)
const menuClose = ref<HTMLButtonElement | null>(null)
let previousOverflow = ''
const pageLabel = computed(() => ({ files: '文件', groups: '群组', devices: '设备', transfers: '传输', history: '传输记录', settings: '设置' })[route.name as string] || '控制台')
async function openNavigation() {
  previousOverflow = document.body.style.overflow
  document.body.style.overflow = 'hidden'
  navigationOpen.value = true
  await nextTick()
  requestAnimationFrame(() => menuClose.value?.focus({ preventScroll: true }))
}
function closeNavigation(restoreFocus = true) {
  if (!navigationOpen.value) return
  navigationOpen.value = false
  document.body.style.overflow = previousOverflow
  if (restoreFocus && isMobile.value) void nextTick(() => menuToggle.value?.focus({ preventScroll: true }))
}
function onViewportChange(event: MediaQueryListEvent) { isMobile.value = event.matches; closeNavigation(false) }
function onNavigationKeydown(event: KeyboardEvent) {
  if (event.defaultPrevented || !isMobile.value || !navigationOpen.value) return
  if (event.key === 'Escape') { event.preventDefault(); closeNavigation(); return }
  if (event.key !== 'Tab') return
  const targets = Array.from(navigation.value?.querySelectorAll<HTMLElement>('button:not(:disabled), a[href]') || []).filter(element => element.getClientRects().length > 0)
  const first = targets[0], last = targets[targets.length - 1]
  if (event.shiftKey && document.activeElement === first) { event.preventDefault(); last?.focus() }
  else if (!event.shiftKey && document.activeElement === last) { event.preventDefault(); first?.focus() }
}
watch(() => route.fullPath, () => closeNavigation())
onMounted(() => mobileQuery.addEventListener('change', onViewportChange))
onBeforeUnmount(() => { mobileQuery.removeEventListener('change', onViewportChange); closeNavigation(false) })

const initials = computed(() => (user.account || '?').slice(0, 2).toUpperCase())

onMounted(() => {
  // A hard reload restores the JWT from localStorage but not the in-memory
  // device identity, so re-claim it before any view needs device headers.
  if (user.isLogin) void user.claimDevice()
})
</script>
