<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { getOAuthProviders, resetPassword, sendEmailCode, startOAuth, toApiError, type OAuthProviderStatus } from '@/api'
import { useUserStore } from '@/stores/user'

type Mode = 'login' | 'signup' | 'forgot'
const user = useUserStore()
const router = useRouter()
const route = useRoute()
const mode = ref<Mode>('login')
const account = ref(user.account)
const email = ref('')
const password = ref('')
const confirm = ref('')
const code = ref('')
const localError = ref('')
const notice = ref('')
const codeBusy = ref(false)
const cooldown = ref(0)
const providers = ref<OAuthProviderStatus[]>([])
let timer = 0

const title = computed(() => mode.value === 'login' ? '登录 MyFolder' : mode.value === 'signup' ? '创建账号' : '找回密码')
const message = computed(() => localError.value || user.error)

function switchMode(next: Mode) {
  mode.value = next
  localError.value = ''
  notice.value = ''
  user.error = ''
  password.value = ''
  confirm.value = ''
  code.value = ''
}

async function requestCode() {
  localError.value = ''
  if (!email.value.trim()) { localError.value = '请先填写邮箱'; return }
  codeBusy.value = true
  try {
    await sendEmailCode(email.value.trim(), mode.value === 'signup' ? 'REGISTER' : 'RESET_PASSWORD')
    notice.value = '验证码已发送，10 分钟内有效'
    cooldown.value = 60
    window.clearInterval(timer)
    timer = window.setInterval(() => { if (--cooldown.value <= 0) window.clearInterval(timer) }, 1000)
  } catch (e) { localError.value = toApiError(e).message }
  finally { codeBusy.value = false }
}

async function submit() {
  localError.value = ''
  notice.value = ''
  if (mode.value === 'login') {
    if (!account.value.trim() || !password.value) { localError.value = '请输入账号/邮箱和密码'; return }
    if (await user.signIn(account.value, password.value)) router.push('/devices')
    return
  }
  if (!email.value.trim() || !code.value.trim() || !password.value) {
    localError.value = '请填写邮箱、验证码和密码'
    return
  }
  if (password.value.length < 8) { localError.value = '密码至少需要 8 个字符'; return }
  if (password.value !== confirm.value) { localError.value = '两次输入的密码不一致'; return }
  if (mode.value === 'signup') {
    if (!account.value.trim()) { localError.value = '请填写账号'; return }
    if (await user.signUp(account.value, password.value, email.value, code.value)) router.push('/devices')
  } else {
    try {
      await resetPassword(email.value, code.value, password.value)
      switchMode('login')
      notice.value = '密码已重置，请使用新密码登录'
    } catch (e) { localError.value = toApiError(e).message }
  }
}

async function oauthLogin(provider: string) {
  localError.value = ''
  try { window.location.assign(await startOAuth(provider)) }
  catch (e) { localError.value = toApiError(e).message }
}

onMounted(async () => {
  if (route.query.reason === 'session-expired') notice.value = '登录状态已失效，请重新登录'
  try { providers.value = await getOAuthProviders() } catch { providers.value = [] }
})
onUnmounted(() => window.clearInterval(timer))
</script>

<template>
  <div class="auth">
    <section class="auth-form">
      <div class="auth-inner auth-v2">
        <div class="brand auth-brand">
          <img class="brand-mark" src="/myfolder-icon-v2.png" alt="" />
          <div class="brand-name">MyFolder</div>
          <div class="brand-ver">1.1.1</div>
        </div>

        <p class="eyebrow">Secure workspace</p>
        <h1 class="h1 auth-title">{{ title }}</h1>
        <p class="sub auth-sub">
          {{ mode === 'login' ? '继续管理你的文件、设备与传输。' : mode === 'signup' ? '邮箱验证后即可创建账号。' : '验证注册邮箱后重置密码。' }}
        </p>

        <form @submit.prevent="submit">
          <template v-if="mode !== 'forgot'">
            <label class="label" for="acct">{{ mode === 'login' ? '账号或邮箱' : '账号' }}</label>
            <div class="field"><input id="acct" v-model="account" autocomplete="username" :disabled="user.busy" /></div>
          </template>

          <template v-if="mode !== 'login'">
            <label class="label field-gap" for="email">邮箱</label>
            <div class="field"><input id="email" v-model="email" type="email" autocomplete="email" placeholder="name@example.com" /></div>
            <label class="label field-gap" for="code">邮箱验证码</label>
            <div class="code-row">
              <div class="field"><input id="code" v-model="code" inputmode="numeric" maxlength="6" autocomplete="one-time-code" /></div>
              <button type="button" class="btn code-button" :disabled="codeBusy || cooldown > 0" @click="requestCode">
                {{ cooldown > 0 ? `${cooldown}s` : codeBusy ? '发送中…' : '获取验证码' }}
              </button>
            </div>
          </template>

          <label class="label field-gap" for="pwd">{{ mode === 'forgot' ? '新密码' : '密码' }}</label>
          <div class="field"><input id="pwd" v-model="password" type="password" :autocomplete="mode === 'login' ? 'current-password' : 'new-password'" /></div>

          <template v-if="mode !== 'login'">
            <label class="label field-gap" for="pwd2">确认密码</label>
            <div class="field"><input id="pwd2" v-model="confirm" type="password" autocomplete="new-password" /></div>
          </template>

          <p v-if="message" class="form-message is-error" role="alert">{{ message }}</p>
          <p v-if="notice" class="form-message is-ok">{{ notice }}</p>

          <button class="btn btn-primary submit-button" :disabled="user.busy">
            {{ user.busy ? '正在处理…' : mode === 'login' ? '登录' : mode === 'signup' ? '注册并登录' : '重置密码' }}
          </button>
        </form>

        <template v-if="mode === 'login'">
          <div class="auth-links"><button class="text-button" @click="switchMode('forgot')">忘记密码？</button><button class="text-button" @click="switchMode('signup')">创建账号</button></div>
          <div class="auth-divider"><span>或使用</span></div>
          <div class="oauth-grid">
            <button v-for="item in providers" :key="item.provider" class="btn oauth-button" :disabled="!item.configured" @click="oauthLogin(item.provider)">
              {{ item.provider === 'nyauth' ? 'Nyauth' : item.provider === 'google' ? 'Google' : 'GitHub' }}
            </button>
          </div>
        </template>
        <button v-else class="text-button back-login" @click="switchMode('login')">返回登录</button>
      </div>
    </section>

    <section class="auth-art">
      <div class="auth-art-copy">
        <div class="eyebrow">Identity · devices · files</div>
        <h1 class="h1">一次验证，安全连接你的每台设备。</h1>
        <p class="sub">邮箱用于找回密码和确认敏感操作；第三方账号可以随时在设置中绑定或解除。</p>
      </div>
      <div class="session-card">
        <span class="eyebrow">Session policy</span>
        <strong>Web · 30 天可续期</strong>
        <strong>PC · 长期设备会话</strong>
        <p class="sub">退出登录或重置密码后可由服务端撤销，不使用无法失效的永久令牌。</p>
      </div>
    </section>
  </div>
</template>

<style scoped>
.auth-v2{max-width:390px}.auth-brand{padding:0;margin-bottom:34px}.auth-title{font-size:25px;margin:9px 0 7px}.auth-sub{margin-bottom:20px}.field-gap{display:block;margin-top:14px}.code-row{display:grid;grid-template-columns:1fr 112px;gap:8px}.code-button{height:38px}.submit-button{width:100%;height:40px;margin-top:18px}.form-message{margin:12px 0 0;font-size:12px}.is-error{color:var(--alert)}.is-ok{color:var(--signal-deep)}.auth-links{display:flex;justify-content:space-between;margin-top:12px}.text-button{border:0;background:none;color:var(--signal-deep);font:inherit;font-size:12px;cursor:pointer;padding:4px 0}.auth-divider{display:flex;align-items:center;gap:10px;color:var(--faint);font-size:11px;margin:17px 0 12px}.auth-divider:before,.auth-divider:after{content:"";height:1px;background:var(--line);flex:1}.oauth-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:7px}.oauth-button{height:38px}.back-login{width:100%;margin-top:14px}.auth-art-copy{position:absolute;inset:52px 48px auto}.auth-art-copy .h1{margin-top:14px;max-width:470px}.auth-art-copy .sub{margin-top:12px;max-width:490px}.session-card{position:absolute;left:48px;right:48px;bottom:52px;border:1px solid rgba(255,255,255,.16);padding:22px;display:grid;gap:10px;background:rgba(255,255,255,.035)}.session-card strong{font-size:16px}.session-card .sub{margin:4px 0 0}@media(max-width:760px){.auth-v2{max-width:none}.oauth-grid{grid-template-columns:1fr}.auth-art{display:none}}
</style>
