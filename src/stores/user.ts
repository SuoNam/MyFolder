import { computed, ref } from 'vue'
import { defineStore } from 'pinia'
import {
  clearCredentials, exchangeOAuth, login as apiLogin, logoutAuth, refreshAuth,
  setAuth, signup as apiSignup, toApiError, type AuthSession,
} from '@/api'
import { currentIdentity, ensureRegistered, forgetIdentity } from '@/lib/device'

const TOKEN_KEY = 'user-token'
const REFRESH_KEY = 'user-refresh-token'
const ACCOUNT_KEY = 'user-account'
const EMAIL_KEY = 'user-email'

export const useUserStore = defineStore('user', () => {
  const account = ref(localStorage.getItem(ACCOUNT_KEY) ?? '')
  const email = ref(localStorage.getItem(EMAIL_KEY) ?? '')
  const token = ref(localStorage.getItem(TOKEN_KEY) ?? '')
  const refreshToken = ref(localStorage.getItem(REFRESH_KEY) ?? '')
  const isLogin = computed(() => !!token.value)
  if (token.value) setAuth(token.value)

  const busy = ref(false)
  const error = ref('')
  const deviceReady = ref(!!currentIdentity())
  const deviceError = ref('')
  let refreshTimer = 0

  function redirectToLogin(reason = 'session-expired') {
    const query = new URLSearchParams({ reason }).toString()
    if (!window.location.pathname.startsWith('/login')) window.location.replace(`/login?${query}`)
  }

  function persist() {
    localStorage.setItem(TOKEN_KEY, token.value)
    localStorage.setItem(REFRESH_KEY, refreshToken.value)
    localStorage.setItem(ACCOUNT_KEY, account.value)
    localStorage.setItem(EMAIL_KEY, email.value)
  }

  function scheduleRefresh(expiresIn: number) {
    window.clearTimeout(refreshTimer)
    refreshTimer = window.setTimeout(() => { void renew() }, Math.max(30_000, (expiresIn - 120) * 1000))
  }

  async function applySession(session: AuthSession) {
    token.value = session.accessToken || session.token
    refreshToken.value = session.refreshToken
    account.value = session.account
    email.value = session.email ?? ''
    setAuth(token.value)
    persist()
    scheduleRefresh(session.expiresIn)
    await claimDevice()
  }

  async function claimDevice() {
    deviceError.value = ''
    try {
      await ensureRegistered(account.value)
      deviceReady.value = true
    } catch (e) {
      deviceReady.value = false
      deviceError.value = toApiError(e).message
    }
  }

  async function authenticate(fn: () => Promise<AuthSession>) {
    busy.value = true
    error.value = ''
    try {
      await applySession(await fn())
      return true
    } catch (e) {
      error.value = toApiError(e).message
      return false
    } finally {
      busy.value = false
    }
  }

  const signIn = (acct: string, password: string) => authenticate(() => apiLogin(acct.trim(), password))
  const signUp = (acct: string, password: string, mail: string, code: string) =>
    authenticate(() => apiSignup(acct.trim(), password, mail.trim(), code.trim()))
  const finishOAuth = (code: string) => authenticate(() => exchangeOAuth(code))

  async function renew() {
    if (!refreshToken.value) return false
    try {
      await applySession(await refreshAuth(refreshToken.value))
      return true
    } catch {
      clearLocalSession()
      redirectToLogin()
      return false
    }
  }

  async function bootstrap() {
    if (!refreshToken.value) return !!token.value
    return renew()
  }

  function clearLocalSession() {
    window.clearTimeout(refreshTimer)
    token.value = ''
    refreshToken.value = ''
    account.value = ''
    email.value = ''
    error.value = ''
    deviceReady.value = false
    localStorage.removeItem(TOKEN_KEY)
    localStorage.removeItem(REFRESH_KEY)
    localStorage.removeItem(ACCOUNT_KEY)
    localStorage.removeItem(EMAIL_KEY)
    forgetIdentity()
    clearCredentials()
  }

  function logout() {
    const oldRefresh = refreshToken.value
    clearLocalSession()
    if (oldRefresh) void logoutAuth(oldRefresh).catch(() => undefined)
  }

  window.addEventListener('myfolder:session-expired', () => {
    clearLocalSession()
    redirectToLogin()
  })

  return {
    account, email, token, refreshToken, isLogin, busy, error, deviceReady, deviceError,
    signIn, signUp, finishOAuth, renew, bootstrap, claimDevice, logout,
  }
})
