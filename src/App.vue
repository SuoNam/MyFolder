<template>
  <div v-if="route.name === 'login'"><RouterView /></div>
  <div v-else class="shell">
    <nav class="rail">
      <div class="brand">
        <img class="brand-mark" src="/myfolder-icon-v2.png" alt="" />
        <div class="brand-name">MyFolder</div>
        <div class="brand-ver">1.1.1</div>
      </div>

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

    <div class="main"><RouterView /></div>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted } from 'vue'
import { RouterLink, RouterView, useRoute } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { useDevicesStore } from '@/stores/devices'
import { useTransfersStore } from '@/stores/transfers'
import Icon from '@/components/Icon.vue'

const route = useRoute()
const user = useUserStore()
const devices = useDevicesStore()
const transfers = useTransfersStore()

const initials = computed(() => (user.account || '?').slice(0, 2).toUpperCase())

onMounted(() => {
  // A hard reload restores the JWT from localStorage but not the in-memory
  // device identity, so re-claim it before any view needs device headers.
  if (user.isLogin) void user.claimDevice()
})
</script>
