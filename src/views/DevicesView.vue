<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useRoute } from 'vue-router'
import Icon from '@/components/Icon.vue'
import SendSheet from '@/components/SendSheet.vue'
import { useDevicesStore } from '@/stores/devices'
import { PHASE_LABEL as phaseLabel, useTransfersStore } from '@/stores/transfers'
import { useUserStore } from '@/stores/user'
import { deviceIcon, formatBytes, formatEta, formatRate, formatWhen } from '@/lib/format'
import { hops, relayVerdict } from '@/lib/route'
import type { ServerDevice } from '@/api'

const devices = useDevicesStore()
const transfers = useTransfersStore()
const user = useUserStore()
const route = useRoute()

const q = ref('')
const showSend = ref(false)
const target = ref<ServerDevice | null>(null)

const shown = computed(() => {
  const needle = q.value.trim().toLowerCase()
  if (!needle) return devices.peers
  return devices.peers.filter(
    (d) =>
      d.deviceName.toLowerCase().includes(needle) ||
      d.os.toLowerCase().includes(needle) ||
      (d.deviceAddress ?? '').toLowerCase().includes(needle),
  )
})

/** Bytes this browser has pushed today, from the local job ledger. */
const sentToday = computed(() => {
  const start = new Date()
  start.setHours(0, 0, 0, 0)
  return transfers.jobs
    .filter((j) => new Date(j.createdAt).getTime() >= start.getTime())
    .reduce((sum, j) => sum + j.bytesSent, 0)
})

function openSend(d?: ServerDevice) {
  target.value = d ?? devices.targets[0] ?? devices.peers[0] ?? null
  showSend.value = true
}

onMounted(() => {
  devices.startPolling()
  transfers.startPolling()
  // ?send=1 deep link: the desktop client's "发送到" menu can open this sheet.
  if (route.query.send) openSend()
})
onUnmounted(() => {
  devices.stopPolling()
  transfers.stopPolling()
})
</script>

<template>
  <header class="topbar">
    <div class="field" style="width: 268px">
      <Icon name="search" />
      <input v-model="q" placeholder="搜索设备" aria-label="搜索设备" />
    </div>
    <div class="spacer" />
    <button class="btn" :disabled="devices.loading" @click="devices.refresh()">
      <Icon name="refresh" />{{ devices.loading ? '刷新中…' : '刷新' }}
    </button>
    <button class="btn btn-primary" :disabled="!devices.peers.length" @click="openSend()">
      <Icon name="send" />发送文件
    </button>
  </header>

  <div class="page">
    <div class="page-head">
      <div>
        <h1 class="h1">设备</h1>
        <p class="sub">登录同一账号的设备会出现在这里。选一台，然后发送文件。</p>
      </div>
    </div>

    <p v-if="devices.error" class="sub" style="margin-bottom: 14px; color: var(--alert)" role="alert">
      {{ devices.error }}
    </p>

    <div class="readouts" style="margin-bottom: 20px">
      <div class="readout">
        <div class="readout-val">{{ devices.peers.length }}</div>
        <div class="readout-lab eyebrow">其他设备</div>
      </div>
      <div class="readout is-live">
        <div class="readout-val">{{ devices.onlineCount }}</div>
        <div class="readout-lab eyebrow">在线</div>
      </div>
      <div class="readout">
        <div class="readout-val">{{ transfers.activeCount }}</div>
        <div class="readout-lab eyebrow">正在传输</div>
      </div>
      <div class="readout">
        <div class="readout-val">{{ formatBytes(sentToday) }}</div>
        <div class="readout-lab eyebrow">今日已发送</div>
      </div>
    </div>

    <!-- This browser, shown separately: it is a send source, never a target. -->
    <article v-if="devices.self" class="card device is-online" style="margin-bottom: 18px">
      <div class="device-top">
        <div class="device-icon"><Icon name="desktop" /></div>
        <div style="flex: 1; min-width: 0">
          <div class="device-name">
            <span class="led is-on" />
            <span>{{ devices.self.deviceName }}</span>
            <span class="device-self">本机</span>
          </div>
          <div class="device-meta">
            {{ devices.self.os }}<br />
            Web 控制台 {{ devices.self.clientVersion }} · 仅作为发送源
          </div>
        </div>
        <span class="chip chip-idle">仅发送</span>
      </div>
      <div class="device-body">
        <p class="mono sub" style="margin: 0">
          浏览器无法接收传入文件，因此不会出现在其他设备的目标列表中。
        </p>
      </div>
    </article>

    <div v-if="!devices.loadedOnce" class="card card-pad">
      <p class="sub" style="margin: 0">正在读取设备列表…</p>
    </div>

    <div v-else-if="!devices.peers.length" class="card card-pad">
      <div class="empty">
        <div class="empty-mark"><Icon name="devices" /></div>
        <div class="empty-title">还没有其他设备</div>
        <p class="empty-sub sub">
          在电脑或手机上安装 MyFolder 客户端并登录同一账号，设备会自动出现在这里。
        </p>
      </div>
    </div>

    <div v-else class="grid-devices">
      <article
        v-for="d in shown"
        :key="d.deviceId"
        class="card device"
        :class="[d.online ? 'is-online' : 'is-offline', target?.deviceId === d.deviceId ? 'is-target' : '']"
      >
        <div class="device-top">
          <div class="device-icon"><Icon :name="deviceIcon(d.deviceType, d.os)" /></div>
          <div style="flex: 1; min-width: 0">
            <div class="device-name">
              <span class="led" :class="d.online ? 'is-on' : ''" />
              <span>{{ d.deviceName }}</span>
            </div>
            <div class="device-meta">
              {{ d.os }}<template v-if="d.deviceAddress"> · {{ d.deviceAddress }}</template><br />
              客户端 {{ d.clientVersion }} · {{ d.online ? '在线' : formatWhen(d.lastSeenAt) }}
            </div>
          </div>
          <span class="chip" :class="d.online ? 'chip-ok' : 'chip-idle'">
            {{ d.online ? '在线' : '离线' }}
          </span>
        </div>

        <div class="device-body">
          <div class="route">
            <div class="route-head">
              <span class="eyebrow">Path</span>
              <span class="mono route-verdict">{{ relayVerdict(d) }}</span>
            </div>
            <ol class="hops">
              <li v-for="h in hops(d)" :key="h.label" class="hop" :class="`is-${h.state}`">
                <i class="hop-node" /><b class="hop-name">{{ h.label }}</b>
                <span class="hop-note">
                  {{ h.note }}<i v-if="h.state === 'active'" class="hop-wire" />
                </span>
              </li>
            </ol>
          </div>
        </div>

        <div class="device-actions">
          <button class="btn btn-primary btn-sm" @click="openSend(d)">
            <Icon name="send" />{{ d.online ? '发送文件' : '发送并等待上线' }}
          </button>
        </div>
      </article>
    </div>

    <div class="page-head" style="margin: 26px 0 12px">
      <div>
        <h2 class="h2">正在传输</h2>
        <p class="sub">Web 控制台经服务器中转发送，上传完成后由目标设备下载。</p>
      </div>
      <div class="spacer" />
      <RouterLink class="btn btn-sm" to="/transfers">全部传输</RouterLink>
    </div>

    <div class="card">
      <div v-if="!transfers.activeJobs.length" class="card-pad">
        <p class="sub" style="margin: 0">当前没有进行中的传输。</p>
      </div>
      <div v-for="j in transfers.activeJobs.slice(0, 3)" :key="j.id" class="xfer">
        <div class="xfer-top">
          <div class="xfer-dir"><Icon name="upload" /></div>
          <div style="min-width: 0">
            <div class="xfer-title">{{ j.title }}</div>
            <div class="xfer-peer">
              {{ j.kind === 'send' ? `发送至 ${j.targetName}` : '上传到服务器' }} ·
              {{ formatBytes(j.totalBytes) }}
            </div>
          </div>
          <div class="xfer-figs">
            <div class="xfer-rate">{{ formatRate(j.rate) }}</div>
            <div class="xfer-eta">{{ formatEta(j.etaSeconds) }}</div>
          </div>
        </div>
        <div class="xfer-bar">
          <span class="chip chip-relay">{{ phaseLabel[j.phase] }}</span>
          <div class="prog is-relay"><span :style="{ width: Math.round(j.fraction * 100) + '%' }" /></div>
          <div class="xfer-pct">{{ Math.round(j.fraction * 100) }}%</div>
        </div>
      </div>
    </div>

    <p v-if="!user.deviceReady && user.deviceError" class="sub" style="margin-top: 14px; color: var(--alert)">
      设备注册未完成：{{ user.deviceError }} — 可在设置页重试。
    </p>
  </div>

  <SendSheet :open="showSend" :target="target" mode="send" @close="showSend = false" />
</template>
