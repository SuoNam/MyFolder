import { ref } from 'vue'
import { defineStore } from 'pinia'
import {
  addGroupMember, createGroup, deleteGroup, listGroups, removeGroupMember,
  renameGroup, toApiError, updateGroupMember, type StorageGroup,
} from '@/api'

export const useGroupsStore = defineStore('groups', () => {
  const groups = ref<StorageGroup[]>([])
  const loading = ref(false)
  const error = ref('')

  async function refresh() {
    loading.value = true
    try { groups.value = await listGroups(); error.value = '' }
    catch (e) { error.value = toApiError(e).message }
    finally { loading.value = false }
  }
  async function create(name: string) {
    try { groups.value.push(await createGroup(name)); error.value = ''; return true }
    catch (e) { error.value = toApiError(e).message; return false }
  }
  async function rename(id: string, name: string) {
    try { replace(await renameGroup(id, name)); error.value = ''; return true }
    catch (e) { error.value = toApiError(e).message; return false }
  }
  async function remove(id: string) {
    try { await deleteGroup(id); groups.value = groups.value.filter((g) => g.groupId !== id); return true }
    catch (e) { error.value = toApiError(e).message; return false }
  }
  async function addMember(id: string, account: string, permission: 'READ' | 'WRITE') {
    try { replace(await addGroupMember(id, account, permission)); error.value = ''; return true }
    catch (e) { error.value = toApiError(e).message; return false }
  }
  async function changeMember(id: string, account: string, permission: 'READ' | 'WRITE') {
    try { replace(await updateGroupMember(id, account, permission)); error.value = ''; return true }
    catch (e) { error.value = toApiError(e).message; return false }
  }
  async function removeMember(id: string, account: string) {
    try { await removeGroupMember(id, account); await refresh(); return true }
    catch (e) { error.value = toApiError(e).message; return false }
  }
  function replace(group: StorageGroup) {
    const index = groups.value.findIndex((g) => g.groupId === group.groupId)
    if (index < 0) groups.value.push(group); else groups.value[index] = group
  }
  return { groups, loading, error, refresh, create, rename, remove, addMember, changeMember, removeMember }
})
