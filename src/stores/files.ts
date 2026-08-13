import { computed, ref } from 'vue'
import { defineStore } from 'pinia'
import {
  copyPaths,
  createFolder,
  deletePaths,
  downloadDirectory,
  downloadFile,
  listFiles,
  movePaths,
  setStorageScope,
  toApiError,
  type FileEntry,
} from '@/api'
import { joinPath } from '@/lib/format'

export interface Crumb {
  name: string
  path: string
}

/**
 * Server-side file browser over POST /file/getfilelist.
 *
 * Paths are relative to the user root; "" is the root itself. The server
 * returns each entry's own `path`, which we use verbatim for delete/download
 * rather than re-deriving it.
 */
export const useFilesStore = defineStore('files', () => {
  const cwd = ref('')
  const entries = ref<FileEntry[]>([])
  const loading = ref(false)
  const error = ref('')
  const loadedOnce = ref(false)
  const busyPath = ref('') // path currently being deleted/downloaded
  const selection = ref<Set<string>>(new Set())
  const scopeType = ref<'PRIVATE' | 'GROUP'>('PRIVATE')
  const scopeId = ref('')
  const scopeName = ref('我的文件')

  const crumbs = computed<Crumb[]>(() => {
    const out: Crumb[] = [{ name: scopeName.value, path: '' }]
    if (!cwd.value) return out
    let acc = ''
    for (const seg of cwd.value.split('/').filter(Boolean)) {
      acc = acc ? `${acc}/${seg}` : seg
      out.push({ name: seg, path: acc })
    }
    return out
  })

  /** Directories first, then files, each alphabetically. */
  const sorted = computed(() =>
    entries.value.slice().sort((a, b) => {
      if (a.type !== b.type) return a.type === 'Directory' ? -1 : 1
      return a.name.localeCompare(b.name, 'zh-Hans-CN')
    }),
  )

  const selectedPaths = computed(() => [...selection.value])
  const hasSelection = computed(() => selection.value.size > 0)

  function toggle(path: string) {
    const next = new Set(selection.value)
    if (next.has(path)) next.delete(path)
    else next.add(path)
    selection.value = next
  }

  function clearSelection() {
    selection.value = new Set()
  }

  function setScope(type: 'PRIVATE' | 'GROUP', id = '', name = '我的文件') {
    scopeType.value = type
    scopeId.value = id
    scopeName.value = type === 'GROUP' ? name : '我的文件'
    setStorageScope(type, id)
    cwd.value = ''
    entries.value = []
    loadedOnce.value = false
    clearSelection()
  }

  async function open(path = '') {
    loading.value = true
    clearSelection()
    try {
      entries.value = await listFiles(path)
      cwd.value = path
      error.value = ''
    } catch (e) {
      error.value = toApiError(e).message
      entries.value = []
    } finally {
      loading.value = false
      loadedOnce.value = true
    }
  }

  const refresh = () => open(cwd.value)

  async function newFolder(name: string) {
    const clean = name.trim()
    if (!clean) return false
    try {
      await createFolder(joinPath(cwd.value, clean))
      await refresh()
      return true
    } catch (e) {
      error.value = toApiError(e).message
      return false
    }
  }

  /**
   * Delete the given paths. Directories are removed recursively by the server.
   *
   * The server aborts the batch on the first failure without rolling back what
   * it already deleted, so we always re-list and report how many items actually
   * survived instead of trusting the response alone.
   */
  async function remove(paths: string[]) {
    if (!paths.length) return false
    busyPath.value = paths.length === 1 ? paths[0] : ''
    const attempted = [...paths]
    // Only paths that were actually present can count as removed later — a path
    // that never existed must not inflate the "已删除 N 项" tally.
    const existedBefore = attempted.filter((p) => entries.value.some((x) => x.path === p))
    try {
      const results = await deletePaths(attempted)
      const failed = results.filter((r) => !r.ok)
      error.value = failed.length ? `${failed.length} 项删除失败：${failed[0].message}` : ''
      await refresh()
      return failed.length === 0
    } catch (e) {
      const err = toApiError(e)
      await refresh()
      // The server aborts mid-batch without rolling back, so anything that was
      // there before and is gone now was genuinely deleted.
      const removed = existedBefore.filter((p) => !entries.value.some((x) => x.path === p)).length
      error.value =
        removed > 0 ? `已删除 ${removed} 项，其余失败：${err.message}` : err.message
      return false
    } finally {
      busyPath.value = ''
    }
  }

  /**
   * True when `name` already exists in the current directory. The server would
   * silently overwrite on move/copy, so every caller checks this first.
   */
  function nameTaken(name: string, ignorePath?: string) {
    const clean = name.trim()
    return entries.value.some((e) => e.name === clean && e.path !== ignorePath)
  }

  /** Rename an entry in place. Refuses to clobber an existing name. */
  async function rename(entry: FileEntry, newName: string) {
    const clean = newName.trim()
    if (!clean || clean === entry.name) return false
    if (/[\\/]/.test(clean)) {
      error.value = '名称不能包含斜杠'
      return false
    }
    if (nameTaken(clean, entry.path)) {
      error.value = `“${clean}” 已存在，请换一个名称`
      return false
    }
    busyPath.value = entry.path
    try {
      const to = joinPath(cwd.value, clean)
      const results = await movePaths([{ from: entry.path, to }])
      const failed = results.filter((r) => !r.ok)
      error.value = failed.length ? `重命名失败：${failed[0].message}` : ''
      await refresh()
      return failed.length === 0
    } catch (e) {
      error.value = toApiError(e).message
      await refresh()
      return false
    } finally {
      busyPath.value = ''
    }
  }

  /**
   * Move or copy the given paths into `destDir` (a path relative to the root).
   * The server does not create missing parents and overwrites collisions, so we
   * verify the destination and check every name before sending anything.
   */
  async function transfer(paths: string[], destDir: string, mode: 'move' | 'copy') {
    if (!paths.length) return false
    const verb = mode === 'move' ? '移动' : '复制'
    const dest = destDir.replace(/^\/+|\/+$/g, '')

    const picked = paths
      .map((p) => entries.value.find((e) => e.path === p))
      .filter((e): e is FileEntry => !!e)
    if (picked.length !== paths.length) {
      error.value = '选中的项已不在当前列表，请刷新后重试'
      return false
    }
    // Moving a directory into itself would destroy it; copy is rejected server
    // side but move is not, so guard both here.
    const selfNest = picked.find(
      (e) => e.type === 'Directory' && (dest === e.path || dest.startsWith(e.path + '/')),
    )
    if (selfNest) {
      error.value = `不能把“${selfNest.name}”${verb}到它自身内部`
      return false
    }
    if (dest === cwd.value) {
      error.value = `目标位置与当前位置相同`
      return false
    }

    busyPath.value = ''
    try {
      // The destination must already exist, and must not already hold these
      // names — the server would overwrite without asking.
      const destEntries = await listFiles(dest)
      const clash = picked.filter((e) => destEntries.some((d) => d.name === e.name))
      if (clash.length) {
        error.value = `目标位置已存在同名项：${clash.map((c) => c.name).join('、')}`
        return false
      }

      const ops = picked.map((e) => ({ from: e.path, to: joinPath(dest, e.name) }))
      const results = mode === 'move' ? await movePaths(ops) : await copyPaths(ops)
      const failed = results.filter((r) => !r.ok)
      error.value = failed.length ? `${failed.length} 项${verb}失败：${failed[0].message}` : ''
      clearSelection()
      await refresh()
      return failed.length === 0
    } catch (e) {
      const err = toApiError(e)
      await refresh()
      // Non-atomic: report what actually left the current directory.
      const moved =
        mode === 'move'
          ? picked.filter((e) => !entries.value.some((x) => x.path === e.path)).length
          : 0
      error.value =
        moved > 0 ? `已${verb} ${moved} 项，其余失败：${err.message}` : `${verb}失败：${err.message}`
      return false
    } finally {
      busyPath.value = ''
    }
  }

  /**
   * Directories the user can pick as a move/copy destination: the root, the
   * current folder's ancestors, and its immediate subfolders. Enough to reach
   * anywhere without building a full tree widget.
   */
  function destinationChoices(exclude: string[] = []): Crumb[] {
    const out: Crumb[] = [{ name: scopeName.value, path: '' }]
    let acc = ''
    for (const seg of cwd.value.split('/').filter(Boolean)) {
      acc = acc ? `${acc}/${seg}` : seg
      out.push({ name: seg, path: acc })
    }
    for (const e of entries.value) {
      if (e.type === 'Directory' && !exclude.includes(e.path)) {
        out.push({ name: `${e.name}/`, path: e.path })
      }
    }
    return out.filter((c) => c.path !== cwd.value)
  }

  /**
   * Download through axios so the Authorization header is attached, then hand
   * the blob to the browser. A plain <a href> would arrive unauthenticated.
   *
   * Directories go through /directory/downloaddirectory and arrive as a ZIP;
   * files go through /file/downloadfile. Both are streamed by the server.
   */
  async function download(entry: FileEntry) {
    busyPath.value = entry.path
    try {
      const { blob, filename } =
        entry.type === 'Directory'
          ? await downloadDirectory(entry.path)
          : await downloadFile(entry.path)
      const url = URL.createObjectURL(blob)
      const a = document.createElement('a')
      a.href = url
      a.download =
        filename || (entry.type === 'Directory' ? `${entry.name}.zip` : entry.name)
      document.body.appendChild(a)
      a.click()
      a.remove()
      // Revoke after the click has been processed.
      setTimeout(() => URL.revokeObjectURL(url), 10_000)
      error.value = ''
      return true
    } catch (e) {
      error.value = toApiError(e).message
      return false
    } finally {
      busyPath.value = ''
    }
  }

  function reset() {
    cwd.value = ''
    entries.value = []
    loadedOnce.value = false
    error.value = ''
    clearSelection()
  }

  return {
    scopeType,
    scopeId,
    scopeName,
    setScope,
    cwd,
    entries,
    sorted,
    crumbs,
    loading,
    loadedOnce,
    error,
    busyPath,
    selection,
    selectedPaths,
    hasSelection,
    toggle,
    clearSelection,
    open,
    refresh,
    newFolder,
    remove,
    nameTaken,
    rename,
    transfer,
    destinationChoices,
    download,
    reset,
  }
})
