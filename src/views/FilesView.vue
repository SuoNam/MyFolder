<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import Icon from '@/components/Icon.vue'
import SendSheet from '@/components/SendSheet.vue'
import { useFilesStore } from '@/stores/files'
import { fileIcon, formatBytes, formatWhen } from '@/lib/format'
import { getFolderAcl, listGroups, setFolderAcl, removeFolderAcl, toApiError, type FileEntry, type FolderAcl, type GroupMember, type FolderPermission } from '@/api'

const files = useFilesStore()
const route = useRoute()
const router = useRouter()

const q = ref('')
const showUpload = ref(false)
const creating = ref(false)
const newName = ref('')
const confirmDelete = ref(false)
const showPermissions = ref(false)
const folderAcl = ref<FolderAcl[]>([])
const groupMembers = ref<GroupMember[]>([])
const aclError = ref('')
const aclDraft = ref<Record<string, FolderPermission>>({})
const isGroupOwner = computed(() => files.scopeType === 'GROUP' && route.query.permission === 'OWNER')
const canWrite = computed(() => files.scopeType !== 'GROUP' || route.query.permission !== 'READ')

/** Inline rename: the path being renamed, and the draft name. */
const renaming = ref('')
const renameDraft = ref('')

/** Move/copy sheet. */
const transferMode = ref<'move' | 'copy' | null>(null)
const destination = ref('')

const transferVerb = computed(() => (transferMode.value === 'copy' ? '复制' : '移动'))
const destinationOptions = computed(() =>
  transferMode.value ? files.destinationChoices(files.selectedPaths) : [],
)

/** Path from the /files/:p(.*)* route param. */
function pathFromRoute(): string {
  const p = route.params.p
  if (Array.isArray(p)) return p.filter(Boolean).join('/')
  return (p as string | undefined) ?? ''
}

const shown = computed(() => {
  const needle = q.value.trim().toLowerCase()
  if (!needle) return files.sorted
  return files.sorted.filter((f) => f.name.toLowerCase().includes(needle))
})

const dirCount = computed(() => files.entries.filter((f) => f.type === 'Directory').length)
const fileCount = computed(() => files.entries.filter((f) => f.type === 'File').length)
const totalSize = computed(() =>
  files.entries.reduce((s, f) => s + (f.type === 'File' ? (f.size ?? 0) : 0), 0),
)

function goto(path: string) {
  const target = path ? `/files/${path.split('/').map(encodeURIComponent).join('/')}` : '/files'
  router.push({ path: target, query: files.scopeType === 'GROUP' ? route.query : {} })
}

function openEntry(entry: FileEntry) {
  if (entry.type === 'Directory') goto(entry.path)
  else void files.download(entry)
}

async function submitFolder() {
  if (await files.newFolder(newName.value)) {
    newName.value = ''
    creating.value = false
  }
}

async function doDelete() {
  await files.remove(files.selectedPaths)
  confirmDelete.value = false
}

async function downloadSelected() {
  // Sequential: parallel blob downloads trip the browser's popup heuristics.
  // Directories are included — they come back as a ZIP.
  for (const path of files.selectedPaths) {
    const entry = files.entries.find((e) => e.path === path)
    if (entry) await files.download(entry)
  }
}

function startRename(entry: FileEntry) {
  renaming.value = entry.path
  renameDraft.value = entry.name
}

async function submitRename(entry: FileEntry) {
  if (await files.rename(entry, renameDraft.value)) renaming.value = ''
}

function openTransfer(mode: 'move' | 'copy') {
  transferMode.value = mode
  destination.value = destinationOptions.value[0]?.path ?? ''
}

async function submitTransfer() {
  if (!transferMode.value) return
  const ok = await files.transfer(files.selectedPaths, destination.value, transferMode.value)
  if (ok) transferMode.value = null
}

async function openPermissions() {
  try {
    const groups = await listGroups()
    groupMembers.value = groups.find((g) => g.groupId === files.scopeId)?.members.filter((m) => m.permission !== 'OWNER') ?? []
    folderAcl.value = await getFolderAcl(files.scopeId, files.cwd)
    aclDraft.value = Object.fromEntries(groupMembers.value.map((m) => [m.account,
      folderAcl.value.find((a) => a.account === m.account)?.permission ?? (m.permission === 'WRITE' ? 'WRITE' : 'READ')])) as Record<string, FolderPermission>
    aclError.value = ''
    showPermissions.value = true
  } catch (e) { aclError.value = toApiError(e).message }
}
async function savePermission(account: string) {
  try { folderAcl.value = await setFolderAcl(files.scopeId, files.cwd, account, aclDraft.value[account]); aclError.value = '' }
  catch (e) { aclError.value = toApiError(e).message }
}
async function inheritPermission(account: string) {
  try { await removeFolderAcl(files.scopeId, files.cwd, account); folderAcl.value = await getFolderAcl(files.scopeId, files.cwd); aclError.value = '' }
  catch (e) { aclError.value = toApiError(e).message }
}

function openRoute() {
  const groupId = typeof route.query.group === 'string' ? route.query.group : ''
  if (groupId) files.setScope('GROUP', groupId, typeof route.query.name === 'string' ? route.query.name : '群组文件')
  else files.setScope('PRIVATE')
  void files.open(pathFromRoute())
}

onMounted(openRoute)
watch(() => route.fullPath, openRoute)
</script>

<template>
  <header class="topbar">
    <div class="field" style="width: 268px">
      <Icon name="search" />
      <input v-model="q" placeholder="搜索当前文件夹" aria-label="搜索文件" />
    </div>
    <div class="spacer" />
    <button class="btn" :disabled="files.loading" @click="files.refresh()">
      <Icon name="refresh" />刷新
    </button>
    <button v-if="isGroupOwner" class="btn" @click="openPermissions"><Icon name="settings" />当前目录权限</button>
    <button class="btn" :disabled="!canWrite" @click="creating = true"><Icon name="plus" />新建文件夹</button>
    <button class="btn btn-primary" :disabled="!canWrite" @click="showUpload = true"><Icon name="upload" />上传</button>
  </header>

  <div class="page">
    <div class="page-head">
      <div>
        <h1 class="h1">文件</h1>
        <p class="sub">从云端选取内容，或发送到你的另一台设备。</p>
      </div>
    </div>

    <p v-if="files.error" class="sub" style="margin-bottom: 14px; color: var(--alert)" role="alert">
      {{ files.error }}
    </p>

    <div class="card">
      <div class="card-head">
        <div class="crumbs">
          <template v-for="(c, i) in files.crumbs" :key="c.path">
            <a v-if="i < files.crumbs.length - 1" href="#" @click.prevent="goto(c.path)">{{ c.name }}</a>
            <b v-else>{{ c.name }}</b>
            <span v-if="i < files.crumbs.length - 1" class="sep">/</span>
          </template>
        </div>
        <div class="spacer" />
        <template v-if="files.hasSelection">
          <span class="mono sub" style="margin-right: 8px">已选 {{ files.selection.size }} 项</span>
          <button class="btn btn-sm" @click="downloadSelected"><Icon name="download" />下载</button>
          <button class="btn btn-sm" @click="openTransfer('move')"><Icon name="transfer" />移动</button>
          <button class="btn btn-sm" @click="openTransfer('copy')"><Icon name="plus" />复制</button>
          <button class="btn btn-sm btn-danger" @click="confirmDelete = true">
            <Icon name="minus" />删除
          </button>
          <button class="btn btn-quiet btn-sm" @click="files.clearSelection()">取消选择</button>
        </template>
        <span v-else class="mono sub">
          {{ dirCount }} 个文件夹 · {{ fileCount }} 个文件 · {{ formatBytes(totalSize) }}
        </span>
      </div>

      <div v-if="creating" class="card-pad" style="border-bottom: 1px solid var(--line-soft)">
        <div style="display: flex; gap: 8px; align-items: center">
          <div class="field" style="flex: 1">
            <Icon name="folder" />
            <input
              v-model="newName"
              placeholder="文件夹名称"
              autofocus
              @keyup.enter="submitFolder"
              @keyup.esc="creating = false"
            />
          </div>
          <button class="btn btn-primary btn-sm" @click="submitFolder">创建</button>
          <button class="btn btn-sm" @click="creating = false">取消</button>
        </div>
      </div>

      <div v-if="!files.loadedOnce" class="card-pad">
        <p class="sub" style="margin: 0">正在读取…</p>
      </div>

      <div v-else-if="!files.entries.length" class="card-pad">
        <div class="empty">
          <div class="empty-mark"><Icon name="folder" /></div>
          <div class="empty-title">这个文件夹是空的</div>
          <p class="empty-sub sub">上传文件，或从客户端发送内容到这里。</p>
        </div>
      </div>

      <table v-else class="files">
        <thead>
          <tr>
            <th style="width: 36px"></th>
            <th>名称</th>
            <th>大小</th>
            <th>修改时间</th>
            <th></th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="f in shown" :key="f.path">
            <td>
              <input
                type="checkbox"
                :checked="files.selection.has(f.path)"
                :aria-label="`选择 ${f.name}`"
                @change="files.toggle(f.path)"
              />
            </td>
            <td>
              <div v-if="renaming === f.path" class="file-cell">
                <div class="file-ico" :class="{ 'is-dir': f.type === 'Directory' }">
                  <Icon :name="fileIcon(f)" />
                </div>
                <div class="field" style="flex: 1">
                  <input
                    v-model="renameDraft"
                    :aria-label="`重命名 ${f.name}`"
                    autofocus
                    @keyup.enter="submitRename(f)"
                    @keyup.esc="renaming = ''"
                  />
                </div>
                <button class="btn btn-sm btn-primary" @click="submitRename(f)">保存</button>
                <button class="btn btn-sm" @click="renaming = ''">取消</button>
              </div>
              <div v-else class="file-cell" style="cursor: pointer" @click="openEntry(f)">
                <div class="file-ico" :class="{ 'is-dir': f.type === 'Directory' }">
                  <Icon :name="fileIcon(f)" />
                </div>
                <span class="file-name">{{ f.name }}</span>
              </div>
            </td>
            <td class="col-size">{{ f.type === 'Directory' ? '—' : formatBytes(f.size) }}</td>
            <td class="col-time">{{ formatWhen(f.lastModified) }}</td>
            <td class="col-act">
              <button
                class="btn btn-sm btn-quiet"
                :aria-label="`重命名 ${f.name}`"
                @click="startRename(f)"
              >
                <Icon name="hash" />
              </button>
              <!-- Directories download as a ZIP, so every row gets this. -->
              <button
                class="btn btn-sm btn-quiet"
                :disabled="files.busyPath === f.path"
                :aria-label="f.type === 'Directory' ? `打包下载 ${f.name}` : `下载 ${f.name}`"
                @click="files.download(f)"
              >
                <Icon name="download" />
              </button>
              <button
                v-if="f.type === 'Directory'"
                class="btn btn-sm btn-quiet"
                :aria-label="`打开 ${f.name}`"
                @click="goto(f.path)"
              >
                <Icon name="chevron" />
              </button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <p class="sub" style="margin-top: 12px">
      发送到其他设备请前往
      <RouterLink to="/devices">设备</RouterLink> 页面选择目标。
    </p>
  </div>

  <!-- Delete confirmation: irreversible, so it is never a one-click action. -->
  <div v-if="confirmDelete" class="scrim" @click.self="confirmDelete = false">
    <section class="sheet" role="dialog" aria-modal="true">
      <div class="sheet-head">
        <p class="eyebrow">Confirm</p>
        <h2 class="h2" style="margin-top: 7px">删除 {{ files.selection.size }} 项？</h2>
        <p class="sub" style="margin-top: 5px">服务器上的文件将被移除，此操作无法撤销。</p>
      </div>
      <div class="sheet-body">
        <div class="picker">
          <div v-for="p in files.selectedPaths" :key="p" class="pick">
            <Icon name="file" />
            <div class="pick-name mono">{{ p }}</div>
          </div>
        </div>
      </div>
      <div class="sheet-foot">
        <button class="btn" @click="confirmDelete = false">取消</button>
        <div class="spacer" />
        <button class="btn btn-danger" @click="doDelete"><Icon name="minus" />确认删除</button>
      </div>
    </section>
  </div>

  <!-- Move / copy: the destination must be picked explicitly, and the store
       refuses any name that already exists there (the server would overwrite). -->
  <div v-if="transferMode" class="scrim" @click.self="transferMode = null">
    <section class="sheet" role="dialog" aria-modal="true">
      <div class="sheet-head">
        <p class="eyebrow">{{ transferMode === 'copy' ? 'Copy' : 'Move' }}</p>
        <h2 class="h2" style="margin-top: 7px">
          {{ transferVerb }} {{ files.selection.size }} 项到
        </h2>
        <p class="sub" style="margin-top: 5px">
          目标位置已存在同名项时不会覆盖，会直接报错并中止。
        </p>
      </div>
      <div class="sheet-body">
        <div v-if="!destinationOptions.length" class="empty" style="padding: 20px 0">
          <div class="empty-title">没有可选的目标位置</div>
          <p class="empty-sub sub">当前文件夹没有子文件夹，也没有上层目录可移动到。</p>
        </div>
        <div v-else class="picker">
          <div
            v-for="d in destinationOptions"
            :key="d.path"
            class="pick"
            :class="{ 'is-picked': destination === d.path }"
            @click="destination = d.path"
          >
            <Icon name="folder" />
            <div style="min-width: 0">
              <div class="pick-name">{{ d.name }}</div>
              <div class="pick-meta mono">{{ d.path || '根目录' }}</div>
            </div>
            <span v-if="destination === d.path" class="pick-mark">✓</span>
          </div>
        </div>

        <label class="label" style="margin-top: 16px">将要{{ transferVerb }}</label>
        <div class="picker">
          <div v-for="p in files.selectedPaths" :key="p" class="pick">
            <Icon name="file" />
            <div class="pick-name mono">{{ p }}</div>
          </div>
        </div>

        <p v-if="files.error" class="sub" style="margin-top: 12px; color: var(--alert)" role="alert">
          {{ files.error }}
        </p>
      </div>
      <div class="sheet-foot">
        <button class="btn" @click="transferMode = null">取消</button>
        <div class="spacer" />
        <button
          class="btn btn-primary"
          :disabled="!destinationOptions.length"
          @click="submitTransfer"
        >
          <Icon name="transfer" />确认{{ transferVerb }}
        </button>
      </div>
    </section>
  </div>

  <SendSheet
    :open="showUpload"
    mode="upload"
    :parent-path="files.cwd"
    @close="showUpload = false"
    @queued="() => router.push('/transfers')"
  />

  <div v-if="showPermissions" class="scrim" @click.self="showPermissions = false">
    <section class="sheet" role="dialog" aria-modal="true">
      <div class="sheet-head"><p class="eyebrow">Folder access</p><h2 class="h2" style="margin-top:7px">当前目录权限</h2><p class="sub" style="margin-top:5px"><span class="mono">{{ files.cwd || '/' }}</span>；未设置时继承上级目录或群组默认权限。</p></div>
      <div class="sheet-body"><p v-if="aclError" class="sub" style="color:var(--alert)">{{ aclError }}</p><div class="picker">
        <div v-for="member in groupMembers" :key="member.account" class="pick"><div><div class="pick-name">{{ member.displayName }}</div><div class="pick-meta mono">{{ member.account }}</div></div><div class="spacer" /><select v-model="aclDraft[member.account]" class="btn btn-sm"><option value="NONE">无权限</option><option value="READ">只读</option><option value="WRITE">可修改</option><option value="MANAGE">可管理</option></select><button class="btn btn-sm btn-primary" @click="savePermission(member.account)">保存</button><button class="btn btn-sm" @click="inheritPermission(member.account)">恢复继承</button></div>
      </div></div>
      <div class="sheet-foot"><div class="spacer" /><button class="btn" @click="showPermissions=false">完成</button></div>
    </section>
  </div>
</template>
