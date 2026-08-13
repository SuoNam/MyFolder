<script setup lang="ts">
import { computed, onMounted, onUnmounted } from 'vue'
import Icon from '@/components/Icon.vue'
import {
  FORWARD_LABEL,
  PHASE_LABEL as phaseLabel,
  useTransfersStore,
  type Job,
} from '@/stores/transfers'
import { useDevicesStore } from '@/stores/devices'
import { formatBytes, formatEta, formatRate, formatWhen } from '@/lib/format'
import type { Forward } from '@/api'

const transfers = useTransfersStore()
const devices = useDevicesStore()

/** Sum of live upload rates — the only rate this console actually measures. */
const totalRate = computed(() => transfers.activeJobs.reduce((s, j) => s + j.rate, 0))

/** Uploaded, but the peer has not finished pulling it from the server yet. */
const waitingCount = computed(() => transfers.liveForwards.length)

const forwardLabel = FORWARD_LABEL

function peerName(f: Forward): string {
  return devices.byId(f.targetDeviceId)?.deviceName ?? f.targetDeviceId
}

function forwardPct(f: Forward): number {
  if (!f.totalBytes) return f.state === 'COMPLETED' ? 100 : 0
  return Math.min(100, Math.round((f.transferredBytes / f.totalBytes) * 100))
}

function progClass(j: Job): string {
  if (j.phase === 'failed') return 'is-fail'
  if (j.phase === 'done') return 'is-done'
  return 'is-relay'
}

onMounted(() => {
  transfers.startPolling()
  devices.startPolling()
})
onUnmounted(() => {
  transfers.stopPolling()
  devices.stopPolling()
})
</script>

<template>
  <header class="topbar">
    <div class="eyebrow">Live queue</div>
    <div class="spacer" />
    <button
      class="btn"
      :disabled="!transfers.doneJobs.length"
      @click="transfers.clearFinished()"
    >
      清除已结束
    </button>
    <RouterLink to="/devices" class="btn btn-primary"><Icon name="send" />发送文件</RouterLink>
  </header>

  <div class="page">
    <div class="page-head">
      <div>
        <h1 class="h1">传输</h1>
        <p class="sub">正在上传的任务、等待对方接收的转发，以及最近的结果。</p>
      </div>
    </div>

    <p v-if="transfers.forwardsError" class="sub" style="margin-bottom: 14px; color: var(--alert)">
      {{ transfers.forwardsError }}
    </p>

    <div class="readouts" style="margin-bottom: 20px">
      <div class="readout" :class="{ 'is-live': totalRate > 0 }">
        <div class="readout-val">{{ formatRate(totalRate) }}</div>
        <div class="readout-lab eyebrow">上传速率</div>
      </div>
      <div class="readout">
        <div class="readout-val">{{ transfers.activeJobs.length }}</div>
        <div class="readout-lab eyebrow">上传中</div>
      </div>
      <div class="readout">
        <div class="readout-val">{{ waitingCount }}</div>
        <div class="readout-lab eyebrow">等待对方接收</div>
      </div>
    </div>

    <section class="card">
      <div class="card-head">
        <h3 class="h3">上传中</h3>
        <div class="spacer" />
        <span class="mono sub">浏览器 → 服务器</span>
      </div>

      <div v-if="!transfers.activeJobs.length" class="card-pad">
        <p class="sub" style="margin: 0">没有正在上传的任务。</p>
      </div>

      <div v-for="j in transfers.activeJobs" :key="j.id" class="xfer">
        <div class="xfer-top">
          <div class="xfer-dir"><Icon name="upload" /></div>
          <div style="min-width: 0">
            <div class="xfer-title">{{ j.title }}</div>
            <div class="xfer-peer">
              {{ j.kind === 'send' ? `发送至 ${j.targetName}` : '上传到服务器' }} ·
              {{ j.fileCount }} 个文件 · {{ formatBytes(j.totalBytes) }}
            </div>
          </div>
          <div class="xfer-figs">
            <div class="xfer-rate">{{ formatRate(j.rate) }}</div>
            <div class="xfer-eta">{{ formatEta(j.etaSeconds) }}</div>
          </div>
        </div>
        <div class="xfer-bar">
          <span class="chip chip-relay">{{ phaseLabel[j.phase] }}</span>
          <div class="prog" :class="progClass(j)">
            <span :style="{ width: Math.round(j.fraction * 100) + '%' }" />
          </div>
          <div class="xfer-pct">{{ Math.round(j.fraction * 100) }}%</div>
          <button class="btn btn-sm btn-quiet" aria-label="取消" @click="transfers.cancelJob(j.id)">
            <Icon name="x" />
          </button>
        </div>
        <p class="mono sub" style="margin: 6px 0 0; font-size: 11px">
          {{ formatBytes(j.bytesSent) }} / {{ formatBytes(j.totalBytes) }}
          <template v-if="j.currentFile"> · {{ j.currentFile }}</template>
        </p>
      </div>
    </section>

    <section class="card" style="margin-top: 14px">
      <div class="card-head">
        <h3 class="h3">等待对方接收</h3>
        <div class="spacer" />
        <span class="mono sub">服务器 → 目标设备</span>
      </div>

      <div v-if="!transfers.liveForwards.length" class="card-pad">
        <p class="sub" style="margin: 0">
          没有待接收的转发。发送完成后，目标设备会从服务器下载文件。
        </p>
      </div>

      <div v-for="f in transfers.liveForwards" :key="f.forwardId" class="xfer">
        <div class="xfer-top">
          <div class="xfer-dir"><Icon name="transfer" /></div>
          <div style="min-width: 0">
            <div class="xfer-title">
              {{ f.files.length === 1 ? f.files[0].path : `${f.files.length} 个文件` }}
            </div>
            <div class="xfer-peer">
              发送至 {{ peerName(f) }} · {{ f.destinationPath }} · {{ formatBytes(f.totalBytes) }}
            </div>
          </div>
          <div class="xfer-figs">
            <div class="xfer-rate">{{ formatBytes(f.transferredBytes) }}</div>
            <div class="xfer-eta">{{ formatWhen(f.updatedAt) }}</div>
          </div>
        </div>
        <div class="xfer-bar">
          <span class="chip chip-relay">{{ forwardLabel[f.state] ?? f.state }}</span>
          <div class="prog is-relay"><span :style="{ width: forwardPct(f) + '%' }" /></div>
          <div class="xfer-pct">{{ forwardPct(f) }}%</div>
          <button
            class="btn btn-sm btn-quiet"
            aria-label="取消转发"
            @click="transfers.cancelForwardById(f.forwardId)"
          >
            <Icon name="x" />
          </button>
        </div>
        <p v-if="f.failureReason" class="sub" style="margin: 6px 0 0; color: var(--alert)">
          {{ f.failureReason }}
        </p>
      </div>
    </section>

    <section v-if="transfers.doneJobs.length" class="card" style="margin-top: 14px">
      <div class="card-head">
        <h3 class="h3">最近结束</h3>
        <div class="spacer" />
        <RouterLink class="btn btn-sm" to="/history">全部记录</RouterLink>
      </div>
      <div v-for="j in transfers.doneJobs.slice(0, 5)" :key="j.id" class="xfer">
        <div class="xfer-top">
          <div class="xfer-dir"><Icon name="upload" /></div>
          <div style="min-width: 0">
            <div class="xfer-title">{{ j.title }}</div>
            <div class="xfer-peer">
              {{ j.kind === 'send' ? `发送至 ${j.targetName}` : '上传到服务器' }} ·
              {{ formatWhen(j.updatedAt) }}
            </div>
          </div>
          <div class="xfer-figs">
            <span
              class="chip"
              :class="j.phase === 'done' ? 'chip-ok' : j.phase === 'failed' ? 'chip-alert' : 'chip-idle'"
            >
              {{ phaseLabel[j.phase] }}
            </span>
          </div>
          <button class="btn btn-sm btn-quiet" aria-label="移除记录" @click="transfers.removeJob(j.id)">
            <Icon name="x" />
          </button>
        </div>
        <p v-if="j.error" class="sub" style="margin: 6px 0 0; color: var(--alert)">{{ j.error }}</p>
      </div>
    </section>
  </div>
</template>
