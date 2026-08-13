import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { fileURLToPath, URL } from 'node:url'

// No dev proxy: the server sends Access-Control-Allow-Headers: * and allows PUT,
// so the browser calls https://api.myfolder.com.cn directly. Override the
// target with VITE_API_BASE at build time.
export default defineConfig({
  plugins: [vue()],
  resolve: { alias: { '@': fileURLToPath(new URL('./src', import.meta.url)) } },
  build: { target: 'es2022' },
})
