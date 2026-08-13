/// <reference types="vite/client" />

interface ImportMetaEnv {
  /** Server base URL, e.g. https://api.myfolder.com.cn */
  readonly VITE_API_BASE?: string
  readonly BASE_URL: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}

declare module '*.vue' {
  import type { DefineComponent } from 'vue'
  const component: DefineComponent<{}, {}, unknown>
  export default component
}

/** Icon.vue inlines the sprite sheet with Vite's ?raw suffix. */
declare module '*.svg?raw' {
  const content: string
  export default content
}
