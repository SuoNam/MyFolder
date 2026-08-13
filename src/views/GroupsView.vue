<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import Icon from '@/components/Icon.vue'
import { useGroupsStore } from '@/stores/groups'
import type { StorageGroup } from '@/api'

const store = useGroupsStore()
const router = useRouter()
const creating = ref(false)
const name = ref('')
const memberAccount = ref<Record<string, string>>({})
const memberPermission = ref<Record<string, 'READ' | 'WRITE'>>({})

onMounted(() => store.refresh())
async function create() {
  if (await store.create(name.value)) { name.value = ''; creating.value = false }
}
function open(group: StorageGroup) {
  router.push({ path: '/files', query: { group: group.groupId, name: group.name, permission: group.myPermission } })
}
async function addMember(group: StorageGroup) {
  const account = (memberAccount.value[group.groupId] || '').trim()
  if (!account) return
  if (await store.addMember(group.groupId, account, memberPermission.value[group.groupId] || 'WRITE')) {
    memberAccount.value[group.groupId] = ''
  }
}
</script>

<template>
  <header class="topbar"><div class="spacer" /><button class="btn btn-primary" @click="creating = true"><Icon name="plus" />新建群组</button></header>
  <div class="page">
    <div class="page-head"><div><h1 class="h1">共享群组</h1><p class="sub">邀请其他账号共同维护一个文件空间，可分别授予只读或读写权限。</p></div></div>
    <p v-if="store.error" class="sub" style="color: var(--alert); margin-bottom: 14px">{{ store.error }}</p>
    <div v-if="creating" class="card card-pad" style="margin-bottom: 14px">
      <label class="label">群组名称</label><div style="display:flex; gap:8px"><div class="field" style="flex:1"><input v-model="name" autofocus placeholder="例如：产品资料" @keyup.enter="create" /></div><button class="btn btn-primary" @click="create">创建</button><button class="btn" @click="creating=false">取消</button></div>
    </div>
    <div v-if="store.loading" class="card card-pad"><p class="sub">正在读取群组…</p></div>
    <div v-else-if="!store.groups.length" class="card card-pad"><div class="empty"><div class="empty-mark"><Icon name="folder" /></div><div class="empty-title">还没有共享群组</div><p class="empty-sub sub">创建群组后即可邀请其他 MyFolder 用户共同读写文件。</p></div></div>
    <div v-else style="display:grid; gap:14px">
      <section v-for="group in store.groups" :key="group.groupId" class="card">
        <div class="card-head"><div><h3 class="h3">{{ group.name }}</h3><p class="sub mono" style="margin-top:3px">{{ group.groupId }}</p></div><div class="spacer" /><span class="tag">{{ group.myPermission === 'OWNER' ? '所有者' : group.myPermission === 'WRITE' ? '可读写' : '只读' }}</span><button class="btn btn-primary btn-sm" @click="open(group)"><Icon name="folder" />打开文件</button></div>
        <div class="card-pad">
          <label class="label">成员</label>
          <div class="picker">
            <div v-for="member in group.members" :key="member.account" class="pick">
              <div class="me-av">{{ member.displayName.slice(0,2).toUpperCase() }}</div><div style="min-width:0"><div class="pick-name">{{ member.displayName }}</div><div class="pick-meta mono">{{ member.account }}</div></div><div class="spacer" />
              <b v-if="member.permission === 'OWNER'" class="tag">所有者</b>
              <select v-else-if="group.myPermission === 'OWNER'" class="btn btn-sm" :value="member.permission" @change="store.changeMember(group.groupId, member.account, ($event.target as HTMLSelectElement).value as 'READ'|'WRITE')"><option value="READ">只读</option><option value="WRITE">可读写</option></select>
              <span v-else class="tag">{{ member.permission === 'WRITE' ? '可读写' : '只读' }}</span>
              <button v-if="group.myPermission === 'OWNER' && member.permission !== 'OWNER'" class="btn btn-sm btn-danger" @click="store.removeMember(group.groupId, member.account)">移除</button>
            </div>
          </div>
          <div v-if="group.myPermission === 'OWNER'" style="display:flex; gap:8px; margin-top:12px"><div class="field" style="flex:1"><input v-model="memberAccount[group.groupId]" placeholder="输入登录账号" @keyup.enter="addMember(group)" /></div><select v-model="memberPermission[group.groupId]" class="btn"><option value="WRITE">可读写</option><option value="READ">只读</option></select><button class="btn" @click="addMember(group)"><Icon name="plus" />添加成员</button></div>
        </div>
      </section>
    </div>
  </div>
</template>
