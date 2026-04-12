<script setup>
import { ref, watch, computed, onMounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useAuth } from './auth'

const router = useRouter()
const route = useRoute()
const isMenuOpen = ref(false)
const isDark = ref(true)

const applyTheme = (dark) => {
  document.documentElement.classList.toggle('light', !dark)
}

onMounted(() => {
  const saved = localStorage.getItem('theme')
  if (saved === 'light') {
    isDark.value = false
  }
  applyTheme(isDark.value)
})

const toggleTheme = () => {
  isDark.value = !isDark.value
  localStorage.setItem('theme', isDark.value ? 'dark' : 'light')
  applyTheme(isDark.value)
}

const {
  currentUser,
  role,
  profile,
  isAdmin,
  isLoggedIn,
  loading,
  error,
  login,
  resetPassword,
  updateProfileData,
  logout,
  startAuthListener,
} = useAuth()

const fullName = computed(() => {
  const first = profile.value?.firstName || ''
  const last = profile.value?.lastName || ''
  const combined = `${first} ${last}`.trim()
  return combined || currentUser.value?.email || ''
})

startAuthListener()

const toggleMenu = () => {
  isMenuOpen.value = !isMenuOpen.value
}

const closeMenu = () => {
  isMenuOpen.value = false
}

const navigateTo = (name) => {
  router.push({ name })
  closeMenu()
}

const handleLogout = async () => {
  closeMenu()
  await logout()
  router.push({ name: 'list' })
}

watch(isAdmin, (next) => {
  if (!next && route.name === 'dashboard') {
    router.push({ name: 'list' })
  }
})

watch(isLoggedIn, (next) => {
  if (next && route.name === 'login') {
    router.push({ name: 'list' })
  }
})
</script>

<template>
  <div id="app" @click="closeMenu">
    <nav class="navigation" @click.stop>
      <div class="nav-header">
        <h1 class="app-title" @click="navigateTo('list')" style="cursor: pointer;">Smart Parkovisko</h1>
        <div class="nav-header-right">
          <button class="theme-toggle" @click.stop="toggleTheme" :title="isDark ? 'Prepnúť na svetlý režim' : 'Prepnúť na tmavý režim'">
            <svg v-if="isDark" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <circle cx="12" cy="12" r="5"/><line x1="12" y1="1" x2="12" y2="3"/><line x1="12" y1="21" x2="12" y2="23"/><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"/><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"/><line x1="1" y1="12" x2="3" y2="12"/><line x1="21" y1="12" x2="23" y2="12"/><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"/><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"/>
            </svg>
            <svg v-else width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"/>
            </svg>
          </button>
          <button class="burger-menu" @click.stop="toggleMenu" :class="{ active: isMenuOpen }">
            <span class="burger-line"></span>
            <span class="burger-line"></span>
            <span class="burger-line"></span>
          </button>
        </div>
      </div>

      <div class="nav-menu" :class="{ open: isMenuOpen }" @click.stop>
        <button
          class="nav-button"
          :class="{ active: route.name === 'list' }"
          @click="navigateTo('list')"
        >
          Parkovisko
        </button>

        <div class="menu-divider"></div>

        <button
          v-if="isAdmin"
          class="nav-button"
          :class="{ active: route.name === 'dashboard' }"
          @click="navigateTo('dashboard')"
        >
          Dashboard
        </button>

        <button
          class="nav-button"
          :class="{ active: route.name === 'account' || route.name === 'login' }"
          @click="navigateTo(isLoggedIn ? 'account' : 'login')"
        >
          {{ isLoggedIn ? (fullName || 'Účet') : 'Prihlásiť sa' }}
        </button>

        <button
          v-if="isLoggedIn"
          class="nav-button logout"
          @click="handleLogout"
          :disabled="loading"
        >
          Odhlásiť
        </button>
      </div>
    </nav>

    <router-view
      v-slot="{ Component }"
    >
      <component
        :is="Component"
        :loading="loading"
        :error="error"
        :onLogin="login"
        :currentUser="currentUser"
        :isAdmin="isAdmin"
        :role="role"
        :profile="profile"
        :onResetPassword="resetPassword"
        :onUpdateProfile="updateProfileData"
      />
    </router-view>
  </div>
</template>

<style scoped>
#app {
  min-height: 100vh;
  width: 100%;
  background: var(--bg-primary);
  margin: 0;
  padding: 0;
  position: relative;
  z-index: 1;
}

.navigation {
  background-color: var(--bg-card);
  border-bottom: 1px solid var(--border-color);
  position: relative;
  z-index: 10000;
  overflow: visible;
  border-radius: 0;
  margin: 0;
  margin-bottom: 0;
}

.nav-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 24px;
  position: relative;
  z-index: 10001;
}

.app-title {
  color: var(--text-primary);
  font-size: 1.25rem;
  font-weight: 600;
  margin: 0;
  letter-spacing: -0.01em;
}

.nav-header-right {
  display: flex;
  align-items: center;
  gap: 12px;
}

.theme-toggle {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  padding: 0;
  border-radius: 8px;
  border: 1px solid var(--border-color);
  background: transparent;
  color: var(--text-secondary);
  cursor: pointer;
  transition: all 0.2s ease;
}

.theme-toggle:hover {
  color: var(--text-primary);
  border-color: var(--border-color-hover);
  background: rgba(255, 255, 255, 0.04);
}

.burger-menu {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  width: 28px;
  height: 20px;
  background: transparent;
  border: none;
  cursor: pointer;
  padding: 0;
}

.burger-line {
  width: 100%;
  height: 2px;
  background-color: var(--text-secondary);
  border-radius: 2px;
  transition: all 0.3s ease;
  transform-origin: center;
}

.burger-menu:hover .burger-line {
  background-color: var(--text-primary);
}

.burger-menu.active .burger-line:nth-child(1) {
  transform: rotate(45deg) translate(6px, 6px);
}

.burger-menu.active .burger-line:nth-child(2) {
  opacity: 0;
}

.burger-menu.active .burger-line:nth-child(3) {
  transform: rotate(-45deg) translate(6px, -6px);
}

.nav-menu {
  display: none;
  flex-direction: column;
  gap: 6px;
  padding: 12px;
  background-color: var(--bg-card);
  border-radius: var(--radius-lg);
  position: absolute;
  top: calc(100% + 8px);
  right: 16px;
  width: 220px;
  z-index: 10002;
  box-shadow: var(--shadow-menu), 0 0 0 1px var(--border-color);
  margin-top: 0;
  pointer-events: auto;
  border: 1px solid var(--border-color);
}

.nav-menu.open {
  display: flex;
}

.nav-button {
  padding: 10px 16px;
  font-size: 0.9rem;
  font-weight: 500;
  border: none;
  background-color: transparent;
  color: var(--text-secondary);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.15s ease;
  text-align: left;
  width: 100%;
  position: relative;
  z-index: 1;
}

.nav-button:hover {
  background-color: rgba(255, 255, 255, 0.06);
  color: var(--text-primary);
}

.nav-button.active {
  background-color: var(--accent-muted);
  color: var(--accent);
  font-weight: 600;
}

.nav-button:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.nav-button.logout {
  color: var(--danger);
}

.nav-button.logout:hover {
  background-color: var(--danger-muted);
}

.menu-divider {
  height: 1px;
  background-color: var(--border-color);
  margin: 4px 0;
}

@media (max-width: 768px) {
  .nav-header {
    padding: 14px 16px;
  }

  .app-title {
    font-size: 1.1rem;
  }

  .nav-menu {
    padding: 10px;
    right: 12px;
    top: calc(100% + 6px);
    width: 200px;
  }

  .nav-button {
    padding: 10px 14px;
    font-size: 0.85rem;
  }
}
</style>