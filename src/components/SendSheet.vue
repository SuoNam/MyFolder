<script setup lang="ts">
/**
 * The one place the console turns local files into a server transfer.
 *
 * Flow: pick files -> pick a target device -> enqueue. The store then runs the
 * server's required order (task -> chunks -> per-file complete -> task complete
 * -> RELAY forward). This sheet never talks to the API directly.
 */
import { computed, ref, watch } from 'vue'
import Icon from '@/components/Icon.vue'
import { useDevicesStore } from '@/stores/devices'
import { useTransfersStore } from '@/stores/transfers'
import { useUserStore } from '@/stores/user'
import { deviceIcon, formatBytes } from '@/lib/format'
import { hops, relayVerdict } from '@/lib/route'
import type { ServerDevice } from '@/api'
import type { UploadFileInput } from '@/lib/upload'

const props = defineProps<{
  open: boolean
  /** Preselected target, e.g. from a device card's 发送 button. */
  target?: ServerDevice | null
  /** Server folder for upload-only mode (the 文件 page passes its cwd). */
  parentPath?: string
  /** 'send' forwards to a peer; 'upload' just stores into the current folder. */
  mode?: 'send' | 'upload'
}>()

const emit = defineEmits<{ close: []; queued: [string] }>()

const devices = useDevicesStore()
const transfers = useTransfersStore()
const user = useUserStore()

const picked = ref<{ path: string; file: File }[]>([])
const target = ref<ServerDevice | null>(null)
const destination = ref('MyFolder')
const hot = ref(false)
const localError = ref('')
const fileInput = ref<HTMLInputElement | null>(null)
const dirInput = ref<HTMLInputElement | null>(null)

const isUploadOnly = computed(() => props.mode === 'upload')
const totalBytes = computed(() => picked.value.reduce((s, p) => s + p.file.size, 0))
const canSubmit = computed(
  () => picked.value.length > 0 && (isUploadOnly.value ? true : !!target.value),
)

watch(
  () => props.open,
  (isOpen) => {
    if (!isOpen) return
    localError.value = ''
    target.value = props.target ?? devices.targets[0] ?? null
    if (!picked.value.length) picked.value = []
  },
)

watch(
  () => props.target,
  (t) => {
    if (t) target.value = t
  },
)

/** Prefer webkitRelativePath so a dropped folder keeps its structure. */
function relPathOf(file: File): string {
  const rel = (file as File & { webkitRelativePath?: string }).webkitRelativePath
  return (rel && rel.length ? rel : file.name).replace(/\\/g, '/')
}

function addFiles(list: FileList | File[] | null) {
  if (!list) return
  mergeEntries(Array.from(list).map((file) => ({ path: relPathOf(file), file })))
}

function onFileInput(e: Event) {
  addFiles((e.target as HTMLInputElement).files)
  ;(e.target as HTMLInputElement).value = ''
}

/** Walk a dropped directory tree so folders keep their relative paths. */
async function walkEntry(entry: FileSystemEntry, prefix: string, out: { path: string; file: File }[]) {
  if (entry.isFile) {
    const file = await new Promise<File | null>((resolve) =>
      (entry as FileSystemFileEntry).file(resolve, () => resolve(null)),
    )
    if (file) out.push({ path: prefix ? `${prefix}/${entry.name}` : entry.name, file })
    return
  }
  const reader = (entry as FileSystemDirectoryEntry).createReader()
  const kids = await new Promise<FileSystemEntry[]>((resolve) => {
    const acc: FileSystemEntry[] = []
    const read = () =>
      reader.readEntries((batch) => {
        if (!batch.length) return resolve(acc)
        acc.push(...batch)
        read()
      }, () => resolve(acc))
    read()
  })
  const next = prefix ? `${prefix}/${entry.name}` : entry.name
  for (const kid of kids) await walkEntry(kid, next, out)
}

/** Merge already-pathed entries, skipping duplicates. */
function mergeEntries(entries: { path: string; file: File }[]) {
  const seen = new Set(picked.value.map((p) => p.path))
  for (const item of entries) {
    if (!seen.has(item.path)) {
      picked.value.push(item)
      seen.add(item.path)
    }
  }
  localError.value = ''
}

async function onDrop(e: DragEvent) {
  hot.value = false
  const items = e.dataTransfer?.items
  // webkitGetAsEntry is the only way to read a dropped *folder*; dataTransfer
  // .files flattens it and drops the directory structure.
  if (items && items.length && typeof items[0].webkitGetAsEntry === 'function') {
    const entries = Array.from(items)
      .map((i) => i.webkitGetAsEntry())
      .filter((x): x is FileSystemEntry => !!x)
    const out: { path: string; file: File }[] = []
    for (const entry of entries) await walkEntry(entry, '', out)
    if (out.length) {
      mergeEntries(out)
      return
    }
  }
  addFiles(e.dataTransfer?.files ?? null)
}

function removeAt(index: number) {
  picked.value.splice(index, 1)
}

function close() {
  emit('close')
}

function submit() {
  if (!canSubmit.value) return
  if (!isUploadOnly.value && !user.deviceReady) {
    localError.value = user.deviceError || '此浏览器尚未注册为设备，无法发送'
    return
  }
  const files: UploadFileInput[] = picked.value.map((p) => ({ path: p.path, blob: p.file }))
  const first = picked.value[0]
  const title =
    picked.value.length === 1
      ? first.path.split('/').pop() || first.path
      : `${first.path.split('/')[0]} · ${picked.value.length} 个文件`

  // Sends go to a per-batch folder so concurrent transfers cannot collide;
  // uploads land directly in the folder the user is looking at.
  const stamp = new Date()
    .toISOString()
    .replace(/[-:T]/g, '')
    .slice(0, 14)

  const id = transfers.enqueue({
    files,
    title,
    ...(isUploadOnly.value
      ? { targetPath: props.parentPath ?? '' }
      : { targetPath: `outbox/send-${stamp}` }),
    target: isUploadOnly.value
      ? null
      : target.value
        ? { deviceId: target.value.deviceId, deviceName: target.value.deviceName }
        : null,
    destinationPath: destination.value,
  })
  if (!id) {
    localError.value = transfers.error || '无法创建任务'
    return
  }
  picked.value = []
  emit('queued', id)
  close()
}
</script>

<template>
  <div v-if="open" class="scrim" @click.self="close">
    <section class="sheet" role="dialog" aria-modal="true" aria-label="新建传输">
      <div class="sheet-head">
        <p class="eyebrow">New transfer</p>
        <h2 class="h2" style="margin-top: 7px">
          {{ isUploadOnly ? '上传到当前文件夹' : '发送到哪台设备' }}
        </h2>
        <p class="sub" style="margin-top: 5px">
          {{
            isUploadOnly
              ? `文件将保存到 ${parentPath ? parentPath : '我的文件'}，全程 SHA-256 校验。`
              : '文件先上传到服务器，再由目标设备下载，离线也能先发出。'
          }}
        </p>
      </div>

      <div class="sheet-body">
        <div
          class="drop"
          :class="{ 'is-hot': hot }"
          @dragover.prevent="hot = true"
          @dragleave.prevent="hot = false"
          @drop.prevent="onDrop"
          @click="fileInput?.click()"
        >
          <div class="empty-mark"><Icon name="upload" /></div>
          <div class="drop-title">拖放文件或文件夹</div>
          <p class="sub drop-sub">也可以点击选择本地内容</p>
        </div>
        <input ref="fileInput" type="file" multiple hidden @change="onFileInput" />
        <input
          ref="dirInput"
          type="file"
          multiple
          hidden
          webkitdirectory
          directory
          @change="onFileInput"
        />
        <div style="display: flex; gap: 8px; margin-top: 10px">
          <button class="btn btn-sm" @click="fileInput?.click()"><Icon name="plus" />选择文件</button>
          <button class="btn btn-sm" @click="dirInput?.click()"><Icon name="folder" />选择文件夹</button>
          <div class="spacer" />
          <span v-if="picked.length" class="mono sub" style="align-self: center">
            {{ picked.length }} 个文件 · {{ formatBytes(totalBytes) }}
          </span>
        </div>

        <div v-if="picked.length" class="picker" style="margin-top: 12px; max-height: 168px; overflow: auto">
          <div v-for="(p, i) in picked" :key="p.path" class="pick">
            <Icon name="file" />
            <div style="min-width: 0">
              <div class="pick-name" style="overflow: hidden; text-overflow: ellipsis">{{ p.path }}</div>
              <div class="pick-meta">{{ formatBytes(p.file.size) }}</div>
            </div>
            <button class="btn btn-quiet btn-sm" aria-label="移除" @click.stop="removeAt(i)">
              <Icon name="x" />
            </button>
          </div>
        </div>

        <template v-if="!isUploadOnly">
          <label class="label" style="margin-top: 16px">目标设备</label>
          <div v-if="!devices.peers.length" class="empty" style="padding: 20px 0">
            <div class="empty-title">还没有其他设备</div>
            <p class="empty-sub sub">在另一台设备上登录同一账号即可出现在这里。</p>
          </div>
          <div v-else class="picker">
            <div
              v-for="d in devices.peers"
              :key="d.deviceId"
              class="pick"
              :class="{ 'is-picked': target?.deviceId === d.deviceId }"
              @click="target = d"
            >
              <span class="led" :class="d.online ? 'is-on' : ''" />
              <Icon :name="deviceIcon(d.deviceType, d.os)" />
              <div style="min-width: 0">
                <div class="pick-name">{{ d.deviceName }}</div>
                <div class="pick-meta">{{ d.os }} · {{ d.online ? '在线' : '离线，将等待上线' }}</div>
              </div>
              <span v-if="target?.deviceId === d.deviceId" class="pick-mark">✓</span>
            </div>
          </div>

          <label class="label" style="margin-top: 16px">对方保存位置</label>
          <div class="field is-path">
            <Icon name="folder" />
            <input v-model="destination" placeholder="MyFolder" />
          </div>

          <div class="route" style="margin-top: 14px">
            <div class="route-head">
              <span class="eyebrow">Path</span>
              <span class="mono route-verdict">{{ relayVerdict(target) }}</span>
            </div>
            <ol class="hops">
              <li v-for="h in hops(target)" :key="h.label" class="hop" :class="`is-${h.state}`">
                <i class="hop-node" /><b class="hop-name">{{ h.label }}</b>
                <span class="hop-note">
                  {{ h.note }}<i v-if="h.state === 'active'" class="hop-wire" />
                </span>
              </li>
            </ol>
          </div>
        </template>

        <p v-if="localError" class="sub" style="margin-top: 12px; color: var(--alert)" role="alert">
          {{ localError }}
        </p>
      </div>

      <div class="sheet-foot">
        <button class="btn" @click="close">取消</button>
        <div class="spacer" />
        <button class="btn btn-primary" :disabled="!canSubmit" @click="submit">
          <Icon name="send" />
          {{ isUploadOnly ? '开始上传' : `发送到 ${target?.deviceName || '设备'}` }}
        </button>
      </div>
    </section>
  </div>
</template>
