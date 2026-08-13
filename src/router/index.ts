import { createRouter, createWebHistory } from 'vue-router'
import { useUserStore } from '@/stores/user'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    { path: '/', redirect: '/devices' },
    { path: '/login', name: 'login', component: () => import('@/views/LoginView.vue') },
    { path: '/oauth/callback', name: 'oauth-callback', component: () => import('@/views/OAuthCallbackView.vue') },
    { path: '/devices', name: 'devices', component: () => import('@/views/DevicesView.vue') },
    { path: '/files/:p(.*)*', name: 'files', component: () => import('@/views/FilesView.vue') },
    { path: '/groups', name: 'groups', component: () => import('@/views/GroupsView.vue') },
    { path: '/transfers', name: 'transfers', component: () => import('@/views/TransfersView.vue') },
    { path: '/history', name: 'history', component: () => import('@/views/HistoryView.vue') },
    { path: '/settings', name: 'settings', component: () => import('@/views/SettingsView.vue') },
  ],
})

router.beforeEach((to) => {
  const user = useUserStore()
  if (to.name !== 'login' && to.name !== 'oauth-callback' && !user.isLogin) return { name: 'login' }
})

export default router
