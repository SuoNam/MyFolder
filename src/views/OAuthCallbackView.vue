<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useUserStore } from '@/stores/user'

const route = useRoute()
const router = useRouter()
const user = useUserStore()
const error = ref('')

onMounted(async () => {
  if (typeof route.query.linked === 'string') {
    await router.replace('/settings')
    return
  }

  const providerError = typeof route.query.oauthError === 'string' ? route.query.oauthError : ''
  const providerDescription = typeof route.query.oauthErrorDescription === 'string'
    ? route.query.oauthErrorDescription
    : ''
  if (providerError) {
    error.value = providerDescription || `第三方登录失败：${providerError}`
    return
  }

  const code = typeof route.query.code === 'string' ? route.query.code : ''
  if (!code) {
    error.value = 'OAuth 登录回调无效'
    return
  }
  if (await user.finishOAuth(code)) await router.replace('/devices')
  else error.value = user.error || '第三方登录失败'
})
</script>

<template>
  <div class="oauth-callback card">
    <img class="brand-mark" src="/myfolder-icon-v2.png" alt="" />
    <h1 class="h1">{{ error ? '无法完成登录' : '正在完成登录…' }}</h1>
    <p class="sub">{{ error || '正在建立 MyFolder 会话，请稍候。' }}</p>
    <button v-if="error" class="btn" @click="router.replace('/login')">返回登录</button>
  </div>
</template>

<style scoped>
.oauth-callback { width:min(420px,calc(100vw - 32px)); margin:15vh auto; padding:30px; display:grid; gap:14px }
.oauth-callback .btn { margin-top:8px }
</style>
