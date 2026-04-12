<script setup>
import { ref, watch } from 'vue'
import { useRouter } from 'vue-router'

const router = useRouter()

const props = defineProps({
  currentUser: { type: Object, default: null },
  role: { type: String, default: '' },
  profile: { type: Object, default: () => ({ firstName: '', lastName: '' }) },
  loading: { type: Boolean, default: false },
  error: { type: String, default: '' },
  onResetPassword: { type: Function, required: true },
  onUpdateProfile: { type: Function, required: true },
})

const resetMessage = ref('')

const firstName = ref(props.profile?.firstName || '')
const lastName = ref(props.profile?.lastName || '')
const saveMessage = ref('')

watch(
  () => props.profile,
  (next) => {
    firstName.value = next?.firstName || ''
    lastName.value = next?.lastName || ''
  },
  { deep: true }
)

const handleReset = async () => {
  resetMessage.value = ''
  const email = props.currentUser?.email
  if (!email) return
  const result = await props.onResetPassword(email)
  if (result) {
    resetMessage.value = 'Email na obnovenie hesla bol odoslaný. Skontroluj svoju schránku.'
    setTimeout(() => { resetMessage.value = '' }, 5000)
  }
}

const handleSave = async () => {
  saveMessage.value = ''
  const result = await props.onUpdateProfile(firstName.value.trim(), lastName.value.trim())
  if (result) {
    saveMessage.value = 'Údaje boli uložené.'
    setTimeout(() => { saveMessage.value = '' }, 4000)
  }
}
</script>

<template>
  <div class="account-page">
    <div v-if="!currentUser" class="login-required">
      <h1>Prihlásenie požadované</h1>
      <p>Musíš sa prihlásiť aby si mohol prezrieť a spravovať svoj účet.</p>
      <div class="action-buttons">
        <button class="login-button" @click="router.push({ name: 'login' })">
          Prihlásiť sa
        </button>
        <button class="go-back-button" @click="router.push({ name: 'list' })">
          ← Vrátiť sa naspäť
        </button>
      </div>
    </div>

    <div v-else class="account-card">
      <h1>Účet</h1>

      <div class="account-row">
        <span class="label">Email:</span>
        <span class="value">{{ currentUser?.email || '—' }}</span>
      </div>

      <h2>Profil</h2>
      <p class="help">Tu môžeš upraviť meno a priezvisko, ktoré sa zobrazí v aplikácii.</p>
      <div class="profile-grid">
        <input
          v-model="firstName"
          type="text"
          placeholder="Meno"
          autocomplete="given-name"
          maxlength="50"
        />
        <input
          v-model="lastName"
          type="text"
          placeholder="Priezvisko"
          autocomplete="family-name"
          maxlength="50"
        />
      </div>
      <button class="save-button" @click="handleSave" :disabled="loading">
        Uložiť
      </button>

      <div class="status">
        <span v-if="saveMessage" class="ok">{{ saveMessage }}</span>
        <span v-else-if="loading" class="info">Prosím čakajte...</span>
      </div>

      <div class="account-divider"></div>

      <h2>Reset hesla</h2>
      <p class="help">Na tvôj email bude odoslaný odkaz na obnovenie hesla.</p>
      <button class="reset-button" @click="handleReset" :disabled="loading">
        Poslať email na reset hesla
      </button>

      <div class="status">
        <span v-if="resetMessage" class="ok">{{ resetMessage }}</span>
        <span v-else-if="error" class="error">{{ error }}</span>
        <span v-else-if="loading" class="info">Prosím čakajte...</span>
      </div>
    </div>
  </div>
</template>

<style scoped>
.account-page {
  padding: 30px 20px;
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
}

.account-card {
  width: min(520px, 100%);
  padding: 32px;
  border-radius: var(--radius-lg);
  background: var(--bg-card);
  border: 1px solid var(--border-color);
  box-shadow: 0 8px 30px rgba(0, 0, 0, 0.3);
  color: var(--text-primary);
}

.account-card h1 {
  margin: 0 0 16px;
  font-size: 1.6rem;
  font-weight: 600;
}

.account-row {
  display: flex;
  justify-content: space-between;
  padding: 10px 0;
  border-bottom: 1px solid var(--border-color);
  font-size: 0.9rem;
}

.label {
  color: var(--text-secondary);
}

.value {
  font-weight: 600;
}

.account-divider {
  margin: 24px 0;
  height: 1px;
  background: var(--border-color);
}

h2 {
  margin: 0 0 10px;
  font-size: 1.15rem;
  font-weight: 600;
}

.help {
  margin: 0 0 18px;
  color: var(--text-secondary);
  font-size: 0.88rem;
  line-height: 1.5;
}

.profile-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
  margin-bottom: 14px;
}

.save-button {
  width: 100%;
  padding: 10px 14px;
  border-radius: var(--radius);
  border: none;
  background: var(--accent);
  color: white;
  font-weight: 600;
  font-size: 0.9rem;
  cursor: pointer;
  transition: all 0.2s ease;
  margin-bottom: 18px;
}

.save-button:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.save-button:hover:not(:disabled) {
  background: var(--accent-hover);
}

input {
  width: 100%;
  padding: 10px 14px;
  border-radius: var(--radius);
  border: 1px solid var(--border-color);
  background: var(--bg-input);
  color: var(--text-primary);
  font-size: 0.9rem;
  margin-bottom: 14px;
  transition: border-color 0.2s;
}

input:focus {
  outline: none;
  border-color: var(--accent);
}

input::placeholder {
  color: var(--text-muted);
}

.reset-button {
  width: 100%;
  padding: 10px 14px;
  border-radius: var(--radius);
  border: 1px solid var(--border-color);
  background: transparent;
  color: var(--text-primary);
  font-weight: 600;
  font-size: 0.9rem;
  cursor: pointer;
  transition: all 0.2s ease;
}

.reset-button:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.reset-button:hover:not(:disabled) {
  background: rgba(255, 255, 255, 0.04);
  border-color: var(--border-color-hover);
}

.status {
  margin-top: 14px;
}

.error {
  color: var(--danger);
  font-weight: 500;
  font-size: 0.9rem;
}

.ok {
  color: var(--success);
  font-weight: 500;
  font-size: 0.9rem;
}

.info {
  color: var(--text-secondary);
  font-weight: 500;
  font-size: 0.9rem;
}

.login-required {
  padding: 40px 20px;
  max-width: 500px;
  margin: 0 auto;
  text-align: center;
  background: var(--bg-card);
  border-radius: var(--radius-lg);
  border: 1px solid var(--border-color);
}

.login-required h1 {
  color: var(--text-primary);
  margin: 0 0 16px;
  font-size: 1.6rem;
  font-weight: 600;
}

.login-required p {
  color: var(--text-secondary);
  margin: 0 0 24px;
  font-size: 0.95rem;
  line-height: 1.6;
}

.action-buttons {
  display: flex;
  gap: 12px;
  justify-content: center;
  flex-direction: column;
}

.login-button {
  padding: 12px 28px;
  border-radius: var(--radius);
  border: none;
  background: var(--accent);
  color: white;
  font-weight: 600;
  font-size: 1rem;
  cursor: pointer;
  transition: all 0.2s ease;
}

.login-button:hover:not(:disabled) {
  background: var(--accent-hover);
}

.go-back-button {
  padding: 10px 24px;
  border-radius: var(--radius);
  border: 1px solid var(--border-color);
  background-color: transparent;
  color: var(--text-secondary);
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s ease;
  font-size: 0.95rem;
}

.go-back-button:hover {
  background-color: rgba(255, 255, 255, 0.04);
  color: var(--text-primary);
}

@media (max-width: 768px) {
  .account-page {
    padding: 20px 12px;
  }

  .account-card {
    padding: 20px 16px;
  }

  .account-card h1 {
    font-size: 1.3rem;
  }

  .profile-grid {
    grid-template-columns: 1fr;
  }

  .login-required {
    padding: 24px 16px;
  }

  .login-required h1 {
    font-size: 1.3rem;
  }
}

@media (max-width: 480px) {
  .account-page {
    padding: 16px 8px;
  }

  .account-card {
    padding: 16px 12px;
  }

  .account-row {
    flex-direction: column;
    gap: 4px;
  }
}
</style>
