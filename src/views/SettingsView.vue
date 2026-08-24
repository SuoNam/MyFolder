<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import Icon from '@/components/Icon.vue'
import { useUserStore } from '@/stores/user'
import { useDevicesStore } from '@/stores/devices'
import { useTransfersStore } from '@/stores/transfers'
import { useFilesStore } from '@/stores/files'
import {
  BASE_URL, getAccountProfile, getOAuthProviders, getStorageUsage, sendEmailCode, startOAuth, updateDisplayName,
  updateTransferPreferences,
  unlinkOAuth, unregisterDevice, verifySensitive, toApiError,
  type AccountProfile, type OAuthProviderStatus, type StorageUsage,
} from '@/api'
import { CLIENT_VERSION, currentIdentity } from '@/lib/device'
import { formatBytes, formatWhen } from '@/lib/format'

const user = useUserStore()
const devices = useDevicesStore()
const transfers = useTransfersStore()
const files = useFilesStore()
const router = useRouter()

const identity = computed(() => currentIdentity())
const initials = computed(() => (profile.value?.displayName || user.account || '?').slice(0, 2).toUpperCase())
const busy = ref(false)
const notice = ref('')
const profile = ref<AccountProfile | null>(null)
const providers = ref<OAuthProviderStatus[]>([])
const bindingProvider = ref('')
const verificationCode = ref('')
const bindingBusy = ref(false)
const bindingError = ref('')
const displayName = ref('')
const nameBusy = ref(false)
const preferenceBusy = ref(false)
const storageUsage = ref<StorageUsage | null>(null)
const storagePercent = computed(() => storageUsage.value?.limitBytes
  ? Math.min(100, storageUsage.value.usedBytes / storageUsage.value.limitBytes * 100) : 0)

async function saveDisplayName() {
  nameBusy.value = true
  notice.value = ''
  try {
    profile.value = await updateDisplayName(displayName.value)
    displayName.value = profile.value.displayName
    notice.value = '用户名已保存'
  } catch (e) { notice.value = toApiError(e).message }
  finally { nameBusy.value = false }
}

async function saveAutoAccept(event: Event) {
  const enabled = (event.target as HTMLInputElement).checked
  preferenceBusy.value = true
  notice.value = ''
  try {
    profile.value = await updateTransferPreferences(enabled)
    notice.value = '自动接收设置已保存'
  } catch (e) { notice.value = toApiError(e).message }
  finally { preferenceBusy.value = false }
}

async function retryRegister() {
  busy.value = true
  notice.value = ''
  await user.claimDevice()
  await devices.refresh()
  busy.value = false
  notice.value = user.deviceReady ? '设备注册已完成' : ''
}

/**
 * Mark this browser offline on the server, then sign out.
 *
 * DELETE /api/v1/devices/{id} only sets online=false — the server keeps the
 * device on the account by design, so this cannot and must not promise removal.
 */
async function unregisterAndLogout() {
  const id = identity.value?.deviceId
  busy.value = true
  try {
    if (id) await unregisterDevice(id)
  } catch (e) {
    notice.value = `注销设备失败：${toApiError(e).message}`
  } finally {
    busy.value = false
  }
  logout()
}

function logout() {
  transfers.reset()
  devices.reset()
  files.reset()
  user.logout()
  router.push('/login')
}

function providerLabel(provider: string) {
  return provider === 'nyauth' ? 'Nyauth' : provider === 'google' ? 'Google' : 'GitHub'
}

function isBound(provider: string) {
  return profile.value?.oauthBindings.some((item) => item.provider === provider) ?? false
}

async function beginBind(provider: string) {
  bindingError.value = ''
  if (!profile.value?.email) { bindingError.value = '请先为账号绑定邮箱'; return }
  bindingBusy.value = true
  try {
    await sendEmailCode(profile.value.email, 'SENSITIVE')
    bindingProvider.value = provider
    verificationCode.value = ''
    notice.value = '验证码已发送到当前账号邮箱'
  } catch (e) { bindingError.value = toApiError(e).message }
  finally { bindingBusy.value = false }
}

async function confirmBind() {
  bindingBusy.value = true
  bindingError.value = ''
  try {
    const action = `oauth:link:${bindingProvider.value}`
    const ticket = await verifySensitive(verificationCode.value, action)
    window.location.assign(await startOAuth(bindingProvider.value, true, ticket))
  } catch (e) { bindingError.value = toApiError(e).message }
  finally { bindingBusy.value = false }
}

async function unbind(provider: string) {
  bindingBusy.value = true
  bindingError.value = ''
  try {
    await unlinkOAuth(provider)
    profile.value = await getAccountProfile()
    notice.value = `已解除 ${providerLabel(provider)} 绑定`
  } catch (e) { bindingError.value = toApiError(e).message }
  finally { bindingBusy.value = false }
}

onMounted(async () => {
  await devices.refresh()
  try {
    [profile.value, providers.value, storageUsage.value] = await Promise.all([getAccountProfile(), getOAuthProviders(), getStorageUsage()])
    displayName.value = profile.value.displayName
  }
  catch (e) { notice.value = toApiError(e).message }
})
</script>

<template>
  <header class="topbar">
    <div class="eyebrow">Preferences</div>
    <div class="spacer" />
    <span v-if="notice" class="sub">{{ notice }}</span>
  </header>

  <div class="page">
    <div class="page-head">
      <div>
        <h1 class="h1">设置</h1>
        <p class="sub">账号、此浏览器的设备身份，以及服务器连接。</p>
      </div>
    </div>

    <div style="display: grid; grid-template-columns: minmax(0, 1fr) minmax(280px, 0.45fr); gap: 14px">
      <div style="display: grid; gap: 14px">
        <section class="card">
          <div class="card-head"><h3 class="h3">此浏览器的设备身份</h3></div>
          <div class="card-pad">
            <p class="sub" style="margin-top: 0">
              为了能向其他设备发送文件，本浏览器会以 <span class="mono">WEB</span> 类型注册为一台设备。
              发送转发时服务器要求设备凭据，因此这一步是必需的。
            </p>

            <label class="label">设备 ID</label>
            <div class="field is-path">
              <Icon name="hash" />
              <input :value="identity?.deviceId ?? '未注册'" readonly />
            </div>

            <label class="label" style="margin-top: 14px">状态</label>
            <div style="display: flex; align-items: center; gap: 9px">
              <span class="led" :class="user.deviceReady ? 'is-on' : ''" />
              <span class="mono">{{ user.deviceReady ? '已注册并保持心跳' : '未注册' }}</span>
              <span v-if="devices.self" class="mono sub">
                · 最近在线 {{ formatWhen(devices.self.lastSeenAt) }}
              </span>
            </div>
            <p v-if="user.deviceError" class="sub" style="margin-top: 8px; color: var(--alert)">
              {{ user.deviceError }}
            </p>
            <button
              v-if="!user.deviceReady"
              class="btn btn-sm"
              style="margin-top: 12px"
              :disabled="busy"
              @click="retryRegister"
            >
              <Icon name="refresh" />重试注册
            </button>
          </div>
        </section>

        <section class="card">
          <div class="card-head"><h3 class="h3">传输方式</h3></div>
          <div class="card-pad">
            <p class="sub" style="margin-top: 0">
              Web 控制台发送的文件先上传到服务器，再由目标设备下载。因此目标设备离线时也能先把文件发出，
              等它上线后自动完成。
            </p>
            <div class="route">
              <div class="route-head">
                <span class="eyebrow">Path</span>
                <span class="mono route-verdict">两跳</span>
              </div>
              <ol class="hops">
                <li class="hop is-wait">
                  <i class="hop-node" /><b class="hop-name">浏览器 → 服务器</b>
                  <span class="hop-note">4 MB 分片 · SHA-256 校验 · 断点续传</span>
                </li>
                <li class="hop is-wait">
                  <i class="hop-node" /><b class="hop-name">服务器 → 目标设备</b>
                  <span class="hop-note">由目标设备拉取，进度可在传输页查看</span>
                </li>
              </ol>
            </div>
          </div>
        </section>

        <section class="card">
          <div class="card-head"><h3 class="h3">接收文件</h3></div>
          <div class="card-pad preference-row">
            <div>
              <b>自动接收其他设备的文件</b>
              <p class="sub">此设置跟随账号，并同步到所有桌面客户端。</p>
            </div>
            <label class="switch" aria-label="自动接收其他设备的文件">
              <input
                type="checkbox"
                role="switch"
                :checked="profile?.autoAcceptDeviceTransfers === true"
                :disabled="preferenceBusy"
                @change="saveAutoAccept"
              >
              <span />
            </label>
          </div>
        </section>

        <section class="card">
          <div class="card-head"><h3 class="h3">服务器</h3></div>
          <div class="card-pad">
            <label class="label">API 地址</label>
            <div class="field is-path">
              <Icon name="server" />
              <input :value="BASE_URL" readonly />
            </div>
            <p class="sub" style="margin-top: 10px">
              构建时通过 <span class="mono">VITE_API_BASE</span> 配置。上传分片 4 MB，
              全程 SHA-256 校验，支持断点续传。
            </p>
          </div>
        </section>
      </div>

      <div style="display: grid; gap: 14px; align-content: start">
        <section v-if="storageUsage" class="card">
          <div class="card-head"><h3 class="h3">存储空间</h3><div class="spacer" /><span class="tag">{{ storageUsage.systemRole === 'SUPER_ADMIN' ? '超级管理员' : storageUsage.tier === 'PREMIUM' ? '高级用户' : '普通用户' }}</span></div>
          <div class="card-pad">
            <div style="display:flex;justify-content:space-between;gap:16px"><b>{{ formatBytes(storageUsage.usedBytes) }} 已使用</b><span class="sub">{{ storageUsage.limitBytes === null ? '不限量' : `共 ${formatBytes(storageUsage.limitBytes)}` }}</span></div>
            <div v-if="storageUsage.limitBytes !== null" style="height:8px;background:var(--sunken);border-radius:99px;overflow:hidden;margin-top:12px"><div :style="{width:`${storagePercent}%`,height:'100%',background:'var(--signal)',borderRadius:'99px'}" /></div>
            <p class="sub" style="margin-top:10px">私人文件与本人贡献到群组的文件共同计入额度；删除后立即释放。</p>
          </div>
        </section>

        <section class="card">
          <div class="card-head"><h3 class="h3">账号</h3></div>
          <div class="card-pad">
            <div class="me">
              <div class="me-av">{{ initials }}</div>
              <div>
                <div class="me-name">{{ profile?.displayName || user.account }}</div>
                <div class="me-sub">{{ devices.peers.length }} 台其他设备</div>
              </div>
            </div>
            <div class="account-email">{{ profile?.email || '未绑定邮箱' }}</div>
            <label class="label account-name-label">用户名</label>
            <div class="account-name-editor">
              <div class="field"><input v-model="displayName" maxlength="40" autocomplete="nickname" /></div>
              <button class="btn btn-sm" :disabled="nameBusy || displayName.trim().length < 2" @click="saveDisplayName">
                {{ nameBusy ? '保存中…' : '保存' }}
              </button>
            </div>
            <div class="account-id">登录账号：<span class="mono">{{ user.account }}</span></div>
            <div class="oauth-bindings">
              <div v-for="provider in providers" :key="provider.provider" class="oauth-binding-row">
                <div>
                  <b>{{ providerLabel(provider.provider) }}</b>
                  <span>{{ isBound(provider.provider) ? '已绑定' : provider.configured ? '未绑定' : '暂未开放' }}</span>
                </div>
                <button
                  v-if="provider.configured"
                  class="btn btn-sm"
                  :disabled="bindingBusy"
                  @click="isBound(provider.provider) ? unbind(provider.provider) : beginBind(provider.provider)"
                >{{ isBound(provider.provider) ? '解除' : '绑定' }}</button>
              </div>
            </div>
            <div v-if="bindingProvider" class="bind-confirm">
              <label class="label">邮箱验证码</label>
              <div class="field"><input v-model="verificationCode" maxlength="6" inputmode="numeric" placeholder="6 位验证码" /></div>
              <div class="bind-actions">
                <button class="btn btn-sm" @click="bindingProvider = ''">取消</button>
                <button class="btn btn-sm btn-primary" :disabled="verificationCode.length !== 6 || bindingBusy" @click="confirmBind">确认并前往授权</button>
              </div>
            </div>
            <p v-if="bindingError" class="sub binding-error">{{ bindingError }}</p>
            <button
              class="btn btn-sm btn-danger"
              style="width: 100%; margin-top: 12px"
              :disabled="busy"
              @click="logout"
            >
              <Icon name="logout" />退出登录
            </button>
            <button
              class="btn btn-sm"
              style="width: 100%; margin-top: 7px"
              :disabled="busy || !identity"
              @click="unregisterAndLogout"
            >
              标记离线并退出
            </button>
            <p class="sub" style="margin-top: 9px; font-size: 11px">
              退出登录会清除本机的设备凭据；“标记离线”还会立即把本机在服务器上置为离线。
              设备会保留在账号的设备列表中，重新登录仍是同一台。
            </p>
          </div>
        </section>

        <section class="card">
          <div class="card-head"><h3 class="h3">关于</h3></div>
          <div class="card-pad">
            <p class="mono">MyFolder Web · v{{ CLIENT_VERSION }}</p>
            <p class="sub">
              Signal Desk UI<br />
              SHA-256 分片与整文件校验
            </p>
          </div>
        </section>
      </div>
    </div>
  </div>
</template>

<style scoped>
.account-email{font-size:12px;color:var(--muted);margin:10px 0 12px}.oauth-bindings{border-top:1px solid var(--line);border-bottom:1px solid var(--line);padding:5px 0;margin-bottom:12px}.oauth-binding-row{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:7px 0}.oauth-binding-row div{display:grid;gap:2px}.oauth-binding-row b{font-size:12px}.oauth-binding-row span{font-size:10px;color:var(--muted)}.bind-confirm{border:1px solid var(--line);background:var(--surface-2);padding:10px;margin-bottom:12px}.bind-actions{display:flex;justify-content:flex-end;gap:7px;margin-top:8px}.binding-error{color:var(--alert);margin:-3px 0 10px}
.account-name-label{display:block;margin-top:14px}.account-name-editor{display:grid;grid-template-columns:1fr auto;gap:7px;align-items:center;margin-top:6px}.account-id{font-size:11px;color:var(--faint);margin:8px 0 12px}
.preference-row{display:flex;align-items:center;justify-content:space-between;gap:18px}.preference-row b{font-size:12px}.preference-row p{margin:4px 0 0}.switch{position:relative;display:inline-flex;width:40px;height:22px;flex:0 0 auto}.switch input{position:absolute;opacity:0;pointer-events:none}.switch span{width:40px;height:22px;border:1px solid var(--line);border-radius:11px;background:var(--sunken);transition:background .15s,border-color .15s}.switch span::after{content:"";display:block;width:16px;height:16px;margin:2px;border-radius:50%;background:var(--surface);border:1px solid var(--line);transition:transform .15s}.switch input:checked+span{background:var(--signal);border-color:var(--signal)}.switch input:checked+span::after{transform:translateX(18px);border-color:transparent}.switch input:focus-visible+span{outline:2px solid var(--focus);outline-offset:2px}.switch input:disabled+span{opacity:.55}
</style>
