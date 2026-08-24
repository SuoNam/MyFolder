<script setup lang="ts">
import { onBeforeUnmount, watch } from 'vue'

const props = withDefaults(defineProps<{
  message: string
  tone?: 'normal' | 'warning' | 'error'
  timeout?: number
}>(), { tone: 'normal', timeout: 5_000 })

const emit = defineEmits<{ dismiss: [] }>()
let timer: number | null = null

function clearTimer() {
  if (timer !== null) window.clearTimeout(timer)
  timer = null
}

watch(() => props.message, (message) => {
  clearTimer()
  if (message) timer = window.setTimeout(() => emit('dismiss'), props.timeout)
}, { immediate: true })

onBeforeUnmount(clearTimer)
</script>

<template>
  <div v-if="message" class="notice-banner" :class="`is-${tone}`" role="status">
    <span>{{ message }}</span>
    <button type="button" aria-label="关闭消息" @click="clearTimer(); emit('dismiss')">×</button>
  </div>
</template>

<style scoped>
.notice-banner{display:flex;align-items:center;gap:10px;min-height:34px;padding:7px 7px 7px 12px;margin-bottom:14px;border:1px solid;border-radius:6px;font-size:12px;line-height:1.45}.notice-banner span{flex:1;min-width:0}.notice-banner button{width:26px;height:26px;border:0;border-radius:5px;background:transparent;color:inherit;font:600 17px/1 sans-serif;cursor:pointer}.notice-banner button:hover{background:rgba(0,0,0,.06)}.is-normal{color:var(--signal-deep);background:var(--signal-wash);border-color:#c4e7d7}.is-warning{color:#9a5d00;background:#fff8e7;border-color:#e8c77c}.is-error{color:var(--alert);background:var(--alert-wash);border-color:#eccec9}
</style>
