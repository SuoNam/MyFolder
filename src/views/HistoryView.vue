<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue'
import Icon from '@/components/Icon.vue'
import NoticeBanner from '@/components/NoticeBanner.vue'
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
  source: '服务器'
  note: string
}

const rows = computed<Row[]>(() => {
  const out: Row[] = []

  for (const f of transfers.historyForwards.filter((item) => ['COMPLETED', 'CANCELLED', 'FAILED', 'REJECTED'].includes(item.state))) {
    const source = devices.byId(f.sourceDeviceId)?.deviceName ?? f.sourceDeviceId
    const target = devices.byId(f.targetDeviceId)?.deviceName ?? f.targetDeviceId
    out.push({
      key: `fw-${f.forwardId}`,
      name: f.files.length === 1 ? f.files[0].path : `${f.files.length} 个文件`,
      flow: `${source} → ${target}`,
      kind: '发送',
      bytes: f.totalBytes,
      when: f.updatedAt,
      result: f.state === 'COMPLETED' ? '成功' : f.state === 'CANCELLED' ? '取消' : '失败',
      source: '服务器',
      note: f.failureReason ?? f.destinationPath,
    })
  }

  for (const task of transfers.uploadHistory.filter((item) => {
    const target = String(item.targetPath ?? '')
    return target !== 'relay' && !target.startsWith('relay/') && ['COMPLETED', 'CANCELLED', 'FAILED'].includes(item.state)
  })) {
    out.push({
      key: `upload-${task.uploadId}`,
      name: task.files.length === 1 ? task.files[0].path : `${task.files.length} 个文件`,
      flow: `账号设备 → 服务器 / ${task.targetPath || '根目录'}`,
      kind: '上传',
      bytes: task.totalBytes,
      when: task.updatedAt,
      result: task.state === 'COMPLETED' ? '成功' : task.state === 'CANCELLED' ? '取消' : '失败',
      source: '服务器',
      note: task.failureReason ?? `${task.files.length} 个文件`,
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
  void transfers.refreshHistory()
  devices.startPolling()
})
onUnmounted(() => devices.stopPolling())
</script>

<template>
  <header class="topbar">
    <div class="field" style="width: 250px">
      <Icon name="search" />
      <input v-model="q" placeholder="搜索文件或设备" aria-label="搜索记录" />
    </div>
    <div class="spacer" />
    <button class="btn" @click="transfers.refreshHistory()">刷新</button>
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
        <p class="sub">同一账号所有设备的客户端收发与服务器上传，统一从服务端读取。</p>
      </div>
    </div>

    <NoticeBanner :message="transfers.historyError" tone="error" @dismiss="transfers.historyError = ''" />

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
        <span class="mono sub" style="font-size: 11px">账号级服务端记录</span>
      </div>
    </div>
  </div>
</template>
