<script setup lang="ts">
/**
 * Transfer history.
 *
 * The server exposes no history endpoint (/api/v1/forwards/history and friends
 * all 404), so this page is assembled from two real sources:
 *   1. forwards currently in a terminal state, from GET /api/v1/forwards
 *   2. this browser's local job ledger in localStorage
 * Both are labelled, and the limitation is stated in the UI rather than hidden
 * behind invented rows.
 */
import { computed, onMounted, onUnmounted, ref } from 'vue'
import Icon from '@/components/Icon.vue'
import { useTransfersStore } from '@/stores/transfers'
import { useDevicesStore } from '@/stores/devices'
import { formatBytes, formatWhen } from '@/lib/format'

const transfers = useTransfersStore()
const devices = useDevicesStore()

const q = ref('')
const onlyFailed = ref(false)

interface Row {
  key: string
  name: string
  flow: string
  /** 发送 = forwarded to a device; 上传 = stored on the server only. */
  kind: '发送' | '上传'
  bytes: number
  when: string
  result: '成功' | '失败' | '取消' | '等待'
  source: '服务器' | '本地记录'
  note: string
}

const rows = computed<Row[]>(() => {
  const out: Row[] = []

  for (const f of transfers.pastForwards) {
    const peer = devices.byId(f.targetDeviceId)?.deviceName ?? f.targetDeviceId
    out.push({
      key: `fw-${f.forwardId}`,
      name: f.files.length === 1 ? f.files[0].path : `${f.files.length} 个文件`,
      flow: `本浏览器 → ${peer}`,
      kind: '发送',
      bytes: f.totalBytes,
      when: f.updatedAt,
      result: f.state === 'COMPLETED' ? '成功' : f.state === 'CANCELLED' ? '取消' : '失败',
      source: '服务器',
      note: f.failureReason ?? f.destinationPath,
    })
  }

  // Local jobs whose forward is not in the server list (upload-only, failed
  // before the forward existed, or a forward the server has already dropped).
  const seen = new Set(transfers.pastForwards.map((f) => f.forwardId))
  const live = new Set(transfers.liveForwards.map((f) => f.forwardId))
  for (const j of transfers.doneJobs) {
    if (j.forwardId && seen.has(j.forwardId)) continue

    // A finished *upload* is not a finished *delivery*. While the forward is
    // still live the transfer is pending, not successful — otherwise this page
    // would contradict the 传输 page for the same transfer.
    let result: Row['result']
    if (j.phase === 'cancelled') result = '取消'
    else if (j.phase === 'failed') result = '失败'
    else if (j.kind === 'send' && j.forwardId && live.has(j.forwardId)) result = '等待'
    else if (j.kind === 'send' && !j.forwardId) result = '失败'
    else result = '成功'

    out.push({
      key: `job-${j.id}`,
      name: j.title,
      flow: j.kind === 'send' ? `本浏览器 → ${j.targetName}` : `本浏览器 → 服务器 ${j.parentPath}`,
      kind: j.kind === 'send' ? '发送' : '上传',
      bytes: j.totalBytes,
      when: j.updatedAt,
      result,
      source: '本地记录',
      note: j.error || (result === '等待' ? '已上传，等待对方接收' : `${j.fileCount} 个文件`),
    })
  }

  const needle = q.value.trim().toLowerCase()
  return out
    .filter((r) => (onlyFailed.value ? r.result === '失败' : true))
    .filter((r) => !needle || r.name.toLowerCase().includes(needle) || r.flow.toLowerCase().includes(needle))
    .sort((a, b) => new Date(b.when).getTime() - new Date(a.when).getTime())
})

const resultChip = (r: Row['result']) =>
  r === '成功' ? 'chip-ok' : r === '失败' ? 'chip-alert' : 'chip-idle'

/** Export exactly what is on screen, so the CSV can't disagree with the table. */
function exportCsv() {
  const head = ['文件', '流向', '类型', '大小(字节)', '时间', '结果', '来源', '备注']
  const body = rows.value.map((r) => [r.name, r.flow, r.kind, r.bytes, r.when, r.result, r.source, r.note])
  const esc = (v: unknown) => `"${String(v).replace(/"/g, '""')}"`
  // Explicit U+FEFF escape so Excel reads the UTF-8 as UTF-8 instead of
  // mojibake. A literal BOM character in source is fragile across tooling.
  const csv = '\uFEFF' + [head, ...body].map((line) => line.map(esc).join(',')).join('\r\n')
  const url = URL.createObjectURL(new Blob([csv], { type: 'text/csv;charset=utf-8' }))
  const a = document.createElement('a')
  a.href = url
  a.download = `myfolder-history-${new Date().toISOString().slice(0, 10)}.csv`
  a.click()
  setTimeout(() => URL.revokeObjectURL(url), 5_000)
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
    <div class="field" style="width: 250px">
      <Icon name="search" />
      <input v-model="q" placeholder="搜索文件或设备" aria-label="搜索记录" />
    </div>
    <div class="spacer" />
    <button class="btn" :class="{ 'btn-primary': onlyFailed }" @click="onlyFailed = !onlyFailed">
      仅看失败
    </button>
    <button class="btn" :disabled="!rows.length" @click="exportCsv">
      <Icon name="download" />导出 CSV
    </button>
  </header>

  <div class="page">
    <div class="page-head">
      <div>
        <h1 class="h1">传输记录</h1>
        <p class="sub">本浏览器发出的传输，以及服务器上仍保留状态的转发。</p>
      </div>
    </div>

    <div class="card">
      <div v-if="!rows.length" class="card-pad">
        <div class="empty">
          <div class="empty-mark"><Icon name="history" /></div>
          <div class="empty-title">还没有记录</div>
          <p class="empty-sub sub">发送一次文件后，结果会出现在这里。</p>
        </div>
      </div>

      <table v-else class="files">
        <thead>
          <tr>
            <th>文件</th>
            <th>设备流向</th>
            <th>类型</th>
            <th>大小</th>
            <th>时间</th>
            <th>结果</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="r in rows" :key="r.key">
            <td>
              <div class="file-cell">
                <div class="file-ico"><Icon name="file" /></div>
                <span class="file-name">{{ r.name }}</span>
              </div>
            </td>
            <td class="mono">{{ r.flow }}</td>
            <td>
              <span class="chip" :class="r.kind === '发送' ? 'chip-relay' : 'chip-idle'">
                {{ r.kind }}
              </span>
            </td>
            <td class="col-size">{{ formatBytes(r.bytes) }}</td>
            <td class="col-time">{{ formatWhen(r.when) }}</td>
            <td>
              <span class="chip" :class="resultChip(r.result)">{{ r.result }}</span>
            </td>
          </tr>
        </tbody>
      </table>

      <div v-if="rows.length" class="card-head">
        <span class="sub">共 {{ rows.length }} 条</span>
        <div class="spacer" />
        <span class="mono sub" style="font-size: 11px">
          服务器未提供历史查询接口，仅能显示当前仍保留的转发状态与本浏览器的本地记录
        </span>
      </div>
    </div>
  </div>
</template>
