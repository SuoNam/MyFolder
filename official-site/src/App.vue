<template>
  <div ref="siteShell" class="site-shell" @pointermove="trackPointer" @pointerleave="resetPointer">
    <div v-if="themeCurtain" class="theme-curtain" :class="themeCurtainTone" aria-hidden="true"></div>
    <div class="scroll-progress" aria-hidden="true"><span :style="{ transform: `scaleX(${scrollProgress})` }"></span></div>
    <div class="ambient-pointer" aria-hidden="true"></div>
    <header class="site-header" :class="{ 'is-scrolled': isScrolled }">
      <a class="brand" href="#top" aria-label="MyFolder 首页" @click="closeMenu">
        <span class="brand-mark" aria-hidden="true">
          <img src="/brand-icon.png" alt="" />
        </span>
        <span class="brand-copy">
          <strong>MyFolder</strong>
          <small>MOVE FILES, NOT LIMITS</small>
        </span>
      </a>

      <nav class="desktop-nav" aria-label="主导航">
        <a href="#features">产品能力</a>
        <a href="#how-it-works">传输方式</a>
        <a href="#security">安全认证</a>
        <a href="#download">下载</a>
      </nav>

      <div class="header-actions">
        <button class="icon-button" :class="{ 'is-theme-animating': themeAnimating }" type="button" :aria-label="darkMode ? '切换浅色模式' : '切换深色模式'" @click="toggleTheme">
          <AppIcon :name="darkMode ? 'sun' : 'moon'" />
        </button>
        <a class="text-link desktop-only" href="https://github.com/SuoNam/MyFolder" target="_blank" rel="noreferrer">
          GitHub <AppIcon name="arrow" />
        </a>
        <a class="button button-small desktop-only" href="https://web.myfolder.com.cn">
          打开 Web 控制台
        </a>
        <button class="menu-button mobile-only" type="button" :aria-expanded="menuOpen" aria-label="打开导航菜单" @click="menuOpen = !menuOpen">
          <AppIcon :name="menuOpen ? 'close' : 'menu'" />
        </button>
      </div>

      <div v-if="menuOpen" class="mobile-menu">
        <a href="#features" @click="closeMenu">产品能力</a>
        <a href="#how-it-works" @click="closeMenu">传输方式</a>
        <a href="#security" @click="closeMenu">安全认证</a>
        <a href="#download" @click="closeMenu">下载</a>
        <a href="https://web.myfolder.com.cn" @click="closeMenu">打开 Web 控制台</a>
        <a href="https://github.com/SuoNam/MyFolder" target="_blank" rel="noreferrer">GitHub</a>
      </div>
    </header>

    <nav class="page-rail" aria-label="页面章节导航">
      <a
        v-for="(page, index) in pageSections"
        :key="page.id"
        :href="`#${page.id}`"
        :class="{ 'is-active': activePageIndex === index }"
        :aria-label="`前往${page.label}`"
        :aria-current="activePageIndex === index ? 'step' : undefined"
      >
        <span>{{ page.label }}</span><i></i>
      </a>
    </nav>

    <main>
      <section id="top" class="hero section-wrap snap-page">
        <div class="hero-copy reveal">
          <div class="eyebrow"><span class="status-dot"></span> v1.1.1 · 现在可用</div>
          <h1>文件应该抵达设备，<br /><em>而不是困在应用里。</em></h1>
          <p class="hero-lede">
            MyFolder 是为多设备而生的文件流转平台。一次选择，智能协商
            <strong>LAN、P2P 或 Server Relay</strong>，让文件沿着当前最合适的路径抵达。
          </p>
          <div class="hero-actions">
            <a class="button" href="#download">下载 v1.1.1 <AppIcon name="download" /></a>
            <a class="button button-ghost" href="https://web.myfolder.com.cn">打开 Web 控制台 <AppIcon name="arrow" /></a>
          </div>
          <div class="hero-meta" aria-label="产品能力摘要">
            <span><AppIcon name="check" /> Windows 桌面端</span>
            <span><AppIcon name="check" /> Linux DEB 客户端</span>
            <span><AppIcon name="check" /> Web 管理</span>
            <span><AppIcon name="check" /> SHA-256 校验</span>
          </div>
        </div>

        <div class="hero-visual reveal reveal-delay-1" aria-label="MyFolder 智能传输示意">
          <div class="visual-orbit orbit-one"></div>
          <div class="visual-orbit orbit-two"></div>
          <div class="device-panel device-source">
            <div class="device-panel-head">
              <span class="device-icon"><AppIcon name="monitor" /></span>
              <span><strong>Studio PC</strong><small>Windows 11 · 在线</small></span>
            </div>
            <div class="file-row">
              <span class="file-type">ZIP</span>
              <span><strong>project-v1.1.1.zip</strong><small>682 MB</small></span>
            </div>
          </div>

          <div class="route-core">
            <img src="/brand-icon.png" alt="MyFolder 图标" />
            <span class="route-pulse"></span>
          </div>

          <div class="route-label route-lan" :class="{ 'is-active': activeRouteIndex === 0 }"><i></i><span>LAN</span><small>优先直连</small></div>
          <div class="route-label route-p2p" :class="{ 'is-active': activeRouteIndex === 1 }"><i></i><span>P2P</span><small>跨网点对点</small></div>
          <div class="route-label route-relay" :class="{ 'is-active': activeRouteIndex === 2 }"><i></i><span>Relay</span><small>稳定回退</small></div>

          <div class="device-panel device-target">
            <div class="device-panel-head">
              <span class="device-icon device-icon-green"><AppIcon name="devices" /></span>
              <span><strong>Work Laptop</strong><small>准备接收</small></span>
            </div>
            <div class="transfer-progress"><span :style="{ width: `${Math.round(transferProgress)}%` }"></span></div>
            <div class="transfer-foot"><span>{{ activeRouteLabel }} 传输中</span><strong>{{ Math.round(transferProgress) }}%</strong></div>
          </div>

          <div class="signal-line line-one"><span></span><span></span></div>
          <div class="signal-line line-two"><span></span><span></span></div>
        </div>
      </section>

      <section class="trust-strip" aria-label="版本特点">
        <div class="section-wrap trust-grid">
          <div><strong>3</strong><span>智能传输通道</span></div>
          <div><strong>4 MB</strong><span>分片上传与恢复</span></div>
          <div><strong>SHA-256</strong><span>文件完整性校验</span></div>
          <div><strong>OAuth 2.0</strong><span>三方账号登录与绑定</span></div>
        </div>
      </section>

      <section class="capability-flow" aria-label="MyFolder 能力流">
        <div class="capability-track">
          <template v-for="copy in 2" :key="copy">
            <span>LAN 直传</span><i></i><span>P2P 穿透</span><i></i><span>Relay 回退</span><i></i>
            <span>断点续传</span><i></i><span>SHA-256 校验</span><i></i><span>跨设备流转</span><i></i>
          </template>
        </div>
      </section>

      <section id="features" class="content-section section-wrap snap-page">
        <div class="section-heading reveal">
          <span class="section-index">01 / PRODUCT</span>
          <div>
            <p class="kicker">一套工具，连接所有设备</p>
            <h2>从“发送文件”到<br />完整的设备流转体验。</h2>
          </div>
          <p>v1.1.1 将设备、传输、账号与 Web 管理整合进统一工作流。你只需要选择目标，路径交给 MyFolder。</p>
        </div>

        <div class="feature-grid">
          <article v-for="(feature, index) in features" :key="feature.title" class="feature-card reveal" :class="`reveal-delay-${(index % 3) + 1}`">
            <div class="feature-top">
              <span class="feature-icon"><AppIcon :name="feature.icon" /></span>
              <span class="feature-number">0{{ index + 1 }}</span>
            </div>
            <h3>{{ feature.title }}</h3>
            <p>{{ feature.description }}</p>
            <ul>
              <li v-for="item in feature.items" :key="item"><span></span>{{ item }}</li>
            </ul>
          </article>
        </div>
      </section>

      <section id="how-it-works" class="route-section snap-page">
        <div class="section-wrap">
          <div class="section-heading section-heading-light reveal">
            <span class="section-index">02 / ROUTING</span>
            <div>
              <p class="kicker">快的时候更快，难的时候依然可靠</p>
              <h2>同一个“发送”，<br />三条不同的路。</h2>
            </div>
            <p>MyFolder 根据设备和网络条件自动协商通道。优先追求直连速度，在网络受限时平滑回退到服务器中转。</p>
          </div>

          <div class="route-table reveal reveal-delay-1">
            <div class="route-table-head">
              <span>通道</span><span>适用场景</span><span>数据路径</span><span>特点</span>
            </div>
            <div v-for="(route, index) in routes" :key="route.name" class="route-table-row" :class="{ 'is-active': activeRouteIndex === index }">
              <span class="route-name"><i :class="route.className"></i>{{ route.name }}</span>
              <span>{{ route.scene }}</span>
              <span>{{ route.path }}</span>
              <span>{{ route.advantage }}</span>
            </div>
          </div>

          <div class="auto-route reveal reveal-delay-2">
            <div class="auto-route-copy">
              <span>自动协商</span>
              <h3>LAN → P2P → Server Relay</h3>
            </div>
            <div class="auto-route-track">
              <span class="track-node node-active">发现设备</span>
              <i></i>
              <span class="track-node node-active">测试直连</span>
              <i></i>
              <span class="track-node">选择路径</span>
              <i></i>
              <span class="track-node">校验抵达</span>
            </div>
          </div>
        </div>
      </section>

      <section id="security" class="content-section section-wrap security-section snap-page">
        <div class="security-visual reveal">
          <div class="security-grid-lines"></div>
          <div class="security-shield"><AppIcon name="shield" /></div>
          <div class="security-badge badge-state">STATE</div>
          <div class="security-badge badge-pkce">PKCE</div>
          <div class="security-badge badge-nonce">NONCE</div>
          <div class="security-badge badge-sha">SHA-256</div>
          <div class="security-ring ring-one"></div>
          <div class="security-ring ring-two"></div>
        </div>

        <div class="security-copy reveal reveal-delay-1">
          <span class="section-index">03 / IDENTITY</span>
          <p class="kicker">登录简单，安全设计不简单</p>
          <h2>账号、安全与设备身份，<br />从一开始就一起设计。</h2>
          <p>支持 Nyauth、Google 与 GitHub OAuth。v1.1.1 引入 state、PKCE、OIDC nonce、邮箱验证码二次认证，以及可撤销的 Access / Refresh Token 会话。</p>
          <div class="provider-row">
            <span class="provider provider-nyauth">N</span>
            <span class="provider provider-google">G</span>
            <span class="provider provider-github"><AppIcon name="github" /></span>
            <span class="provider-label">登录 · 绑定 · 解绑</span>
          </div>
          <div class="security-list">
            <div><AppIcon name="key" /><span><strong>桌面端 OAuth 回调</strong><small>myfolder://oauth/callback</small></span></div>
            <div><AppIcon name="shield" /><span><strong>敏感操作再验证</strong><small>邮箱验证码 + 可撤销会话</small></span></div>
          </div>
        </div>
      </section>

      <section id="download" class="download-section snap-page">
        <div class="section-wrap download-layout">
          <div class="download-copy reveal">
            <span class="section-index">04 / GET MYFOLDER</span>
            <p class="kicker">MyFolder v1.1.1</p>
            <h2>让下一次文件传输，<br />少一步绕路。</h2>
            <p>Windows 与 Linux 桌面端负责设备注册、文件发送和端到端传输；Web 控制台用于跨设备管理、文件浏览与传输追踪。</p>
          </div>

          <div class="download-cards">
            <article class="download-card download-primary reveal reveal-delay-1">
              <span class="download-platform"><AppIcon name="monitor" /> WINDOWS CLIENT</span>
              <h3>桌面客户端</h3>
              <p>Qt 6 桌面应用，支持右键菜单、Shell Extension、LAN / P2P / Relay 传输与桌面 OAuth 回调。</p>
              <div class="download-card-meta"><span>v1.1.1</span><span>Windows 10 / 11</span></div>
              <a class="button button-dark" href="https://github.com/SuoNam/MyFolder/releases/tag/v1.1.1" target="_blank" rel="noreferrer">下载 Windows 版本 <AppIcon name="download" /></a>
            </article>

            <article class="download-card download-linux reveal reveal-delay-2">
              <span class="download-platform"><AppIcon name="monitor" /> LINUX CLIENT</span>
              <h3>Linux 客户端</h3>
              <p>原生 DEB 安装包，支持 Ubuntu / Debian、LAN / P2P / Relay 传输、SHA-256 校验与断点恢复。</p>
              <div class="download-card-meta"><span>v1.1.1</span><span>Debian / Ubuntu · amd64</span></div>
              <a class="button button-outline-dark" href="https://github.com/SuoNam/MyFolder/releases/tag/v1.1.1" target="_blank" rel="noreferrer">下载 Linux DEB <AppIcon name="download" /></a>
            </article>

            <article class="download-card download-web reveal reveal-delay-3">
              <span class="download-platform"><AppIcon name="devices" /> WEB CONSOLE</span>
              <h3>无需安装</h3>
              <p>在浏览器中登录账号，管理设备、文件、群组、传输任务、历史记录与 OAuth 绑定。</p>
              <div class="download-card-meta"><span>在线使用</span><span>响应式界面</span></div>
              <a class="button button-outline-dark" href="https://web.myfolder.com.cn">打开 Web 控制台 <AppIcon name="arrow" /></a>
            </article>
          </div>
        </div>
      </section>

      <section id="open-source" class="open-source-section section-wrap reveal snap-page">
        <div>
          <span class="section-index">BUILD IN THE OPEN</span>
          <h2>源码、协议与版本记录，<br />都在同一个仓库。</h2>
        </div>
        <div class="repo-links">
          <a href="https://github.com/SuoNam/MyFolder/tree/main" target="_blank" rel="noreferrer"><span>Server + Build</span><AppIcon name="arrow" /></a>
          <a href="https://github.com/SuoNam/MyFolder/tree/web" target="_blank" rel="noreferrer"><span>Web Source</span><AppIcon name="arrow" /></a>
          <a href="https://github.com/SuoNam/MyFolder/tree/client" target="_blank" rel="noreferrer"><span>Desktop Client</span><AppIcon name="arrow" /></a>
        </div>
      </section>
    </main>

    <footer class="site-footer">
      <div class="section-wrap footer-grid">
        <div class="footer-brand">
          <div class="brand">
            <span class="brand-mark"><img src="/brand-icon.png" alt="" /></span>
            <span class="brand-copy"><strong>MyFolder</strong><small>MOVE FILES, NOT LIMITS</small></span>
          </div>
          <p>让文件在你的设备之间自然流动。</p>
        </div>
        <div class="footer-links">
          <div><strong>产品</strong><a href="#features">产品能力</a><a href="#download">下载</a><a href="https://web.myfolder.com.cn">Web 控制台</a></div>
          <div><strong>开发</strong><a href="https://github.com/SuoNam/MyFolder">GitHub</a><a href="https://api.myfolder.com.cn/swagger-ui.html">API 文档</a><a href="https://github.com/SuoNam/MyFolder/issues">问题反馈</a></div>
        </div>
      </div>
      <div class="section-wrap footer-bottom"><span>© {{ currentYear }} MyFolder</span><span>Current release · v1.1.1</span></div>
    </footer>
  </div>
</template>

<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import AppIcon from './components/AppIcon.vue'

const menuOpen = ref(false)
const isScrolled = ref(false)
const darkMode = ref(false)
const themeAnimating = ref(false)
const themeCurtain = ref(false)
const themeCurtainTone = ref('to-dark')
const siteShell = ref(null)
const scrollProgress = ref(0)
const activeRouteIndex = ref(0)
const activePageIndex = ref(0)
const transferProgress = ref(18)
const currentYear = new Date().getFullYear()

const pageSections = [
  { id: 'top', label: '首页' },
  { id: 'features', label: '产品' },
  { id: 'how-it-works', label: '路由' },
  { id: 'security', label: '安全' },
  { id: 'download', label: '下载' },
  { id: 'open-source', label: '开源' },
]

const features = [
  {
    icon: 'devices',
    title: '多设备管理',
    description: '同一账号下统一查看和维护你的设备网络。',
    items: ['在线状态与最近活动', '设备重命名、认领与移除', '系统、IP 与客户端版本信息'],
  },
  {
    icon: 'route',
    title: '智能文件路由',
    description: '不要求用户理解网络，自动选择当前最合适的通道。',
    items: ['LAN 局域网高速直传', 'P2P 跨网点对点连接', 'Server Relay 稳定中转回退'],
  },
  {
    icon: 'shield',
    title: '可靠传输',
    description: '从任务建立到文件落盘，持续追踪传输完整性。',
    items: ['SHA-256 文件校验', '断点续传与失败重试', '进度、历史和失败原因'],
  },
  {
    icon: 'key',
    title: '现代账号认证',
    description: '统一账号连接桌面端与 Web，兼顾便捷与安全。',
    items: ['Nyauth / Google / GitHub OAuth', '邮箱验证码二次认证', 'Access / Refresh Token 会话'],
  },
]

const routes = [
  { name: 'LAN', className: 'dot-green', scene: '同一局域网', path: '设备 → 设备', advantage: '低延迟、高吞吐' },
  { name: 'P2P', className: 'dot-blue', scene: '不同网络且可打洞', path: '设备 ⇄ 设备', advantage: '跨网直连、减少中转' },
  { name: 'Server Relay', className: 'dot-amber', scene: '直连不可用', path: '设备 → 云端 → 设备', advantage: '稳定可达、支持恢复' },
]

const activeRouteLabel = computed(() => routes[activeRouteIndex.value]?.name ?? 'LAN')

function closeMenu() {
  menuOpen.value = false
}

function updateScroll() {
  isScrolled.value = window.scrollY > 20
  const scrollable = document.documentElement.scrollHeight - window.innerHeight
  scrollProgress.value = scrollable > 0 ? Math.min(1, window.scrollY / scrollable) : 0
}

function trackPointer(event) {
  if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) return
  siteShell.value?.style.setProperty('--pointer-x', `${event.clientX}px`)
  siteShell.value?.style.setProperty('--pointer-y', `${event.clientY}px`)
  siteShell.value?.classList.add('has-pointer')
}

function resetPointer() {
  siteShell.value?.classList.remove('has-pointer')
}

function handleWheel(event) {
  if (window.innerWidth <= 900) return
  if (event.ctrlKey || event.metaKey || event.altKey) return

  const direction = Math.sign(event.deltaY)
  if (!direction) return
  const currentScroll = window.scrollY
  const currentIndex = pageSections.reduce((closestIndex, page, index) => {
    const element = document.getElementById(page.id)
    const closestElement = document.getElementById(pageSections[closestIndex].id)
    if (!element || !closestElement) return closestIndex
    return Math.abs(element.offsetTop - currentScroll) < Math.abs(closestElement.offsetTop - currentScroll) ? index : closestIndex
  }, 0)
  const nextIndex = currentIndex + direction
  if (nextIndex < 0 || nextIndex >= pageSections.length) return

  event.preventDefault()
  if (wheelLocked) return

  wheelLocked = true
  activePageIndex.value = nextIndex
  const prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches
  document.getElementById(pageSections[nextIndex].id)?.scrollIntoView({ behavior: prefersReducedMotion ? 'auto' : 'smooth', block: 'start' })
  window.clearTimeout(wheelUnlockTimer)
  wheelUnlockTimer = window.setTimeout(() => { wheelLocked = false }, prefersReducedMotion ? 420 : 820)
}

function applyTheme(nextDark) {
  darkMode.value = nextDark
  document.documentElement.dataset.theme = nextDark ? 'dark' : 'light'
  localStorage.setItem('myfolder-theme', nextDark ? 'dark' : 'light')
}

function toggleTheme(event) {
  if (themeAnimating.value) return

  const nextDark = !darkMode.value
  const buttonRect = event.currentTarget?.getBoundingClientRect()
  const originX = event.clientX || (buttonRect ? buttonRect.left + buttonRect.width / 2 : window.innerWidth - 60)
  const originY = event.clientY || (buttonRect ? buttonRect.top + buttonRect.height / 2 : 42)
  document.documentElement.style.setProperty('--theme-x', `${originX}px`)
  document.documentElement.style.setProperty('--theme-y', `${originY}px`)

  if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
    applyTheme(nextDark)
    return
  }

  themeAnimating.value = true
  if (document.startViewTransition) {
    const transition = document.startViewTransition(() => applyTheme(nextDark))
    transition.finished.finally(() => { themeAnimating.value = false })
    return
  }

  themeCurtainTone.value = nextDark ? 'to-dark' : 'to-light'
  themeCurtain.value = true
  window.setTimeout(() => applyTheme(nextDark), 280)
  window.setTimeout(() => {
    themeCurtain.value = false
    themeAnimating.value = false
  }, 640)
}

let observer
let pageObserver
let transferTimer
let wheelUnlockTimer
let wheelLocked = false
onMounted(() => {
  const savedTheme = localStorage.getItem('myfolder-theme')
  const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches
  darkMode.value = savedTheme ? savedTheme === 'dark' : prefersDark
  document.documentElement.dataset.theme = darkMode.value ? 'dark' : 'light'

  updateScroll()
  window.addEventListener('scroll', updateScroll, { passive: true })
  window.addEventListener('wheel', handleWheel, { passive: false })

  observer = new IntersectionObserver((entries) => {
    entries.forEach((entry) => {
      if (entry.isIntersecting) entry.target.classList.add('is-visible')
    })
  }, { threshold: 0.12 })
  document.querySelectorAll('.reveal').forEach((element) => observer.observe(element))

  pageObserver = new IntersectionObserver((entries) => {
    const visiblePage = entries
      .filter((entry) => entry.isIntersecting)
      .sort((left, right) => right.intersectionRatio - left.intersectionRatio)[0]
    if (!visiblePage) return
    const index = pageSections.findIndex((page) => page.id === visiblePage.target.id)
    if (index >= 0) activePageIndex.value = index
  }, { threshold: [0.22, 0.4, 0.62] })
  document.querySelectorAll('.snap-page').forEach((element) => pageObserver.observe(element))

  if (!window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
    transferTimer = window.setInterval(() => {
      transferProgress.value += 0.65
      if (transferProgress.value >= 100) {
        transferProgress.value = 12
        activeRouteIndex.value = (activeRouteIndex.value + 1) % routes.length
      }
    }, 70)
  }
})

onBeforeUnmount(() => {
  window.removeEventListener('scroll', updateScroll)
  window.removeEventListener('wheel', handleWheel)
  window.clearInterval(transferTimer)
  window.clearTimeout(wheelUnlockTimer)
  observer?.disconnect()
  pageObserver?.disconnect()
})
</script>
