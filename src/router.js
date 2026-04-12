import { createRouter, createWebHistory } from 'vue-router'
import ParkingData from './components/ParkingData.vue'
import AuthPage from './components/AuthPage.vue'
import AccountPage from './components/AccountPage.vue'
import AdminDashboard from './components/AdminDashboard.vue'
import { useAuth } from './auth'

const routes = [
  { path: '/', name: 'list', component: ParkingData },
  { path: '/login', name: 'login', component: AuthPage },
  { path: '/account', name: 'account', component: AccountPage, meta: { requiresAuth: true } },
  { path: '/dashboard', name: 'dashboard', component: AdminDashboard, meta: { requiresAuth: true, requiresAdmin: true } },
  { path: '/:pathMatch(.*)*', redirect: '/' },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

router.beforeEach(async (to, from, next) => {
  const { isLoggedIn, isAdmin, authReadyPromise } = useAuth()

  await authReadyPromise

  if (to.name === 'login' && isLoggedIn.value) {
    return next({ name: 'list' })
  }

  if (to.meta.requiresAuth && !isLoggedIn.value) {
    return next({ name: 'login' })
  }

  if (to.meta.requiresAdmin && !isAdmin.value) {
    return next({ name: 'list' })
  }

  next()
})

export default router
