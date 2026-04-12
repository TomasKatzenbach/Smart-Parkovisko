<script setup>
import { ref } from 'vue'

const props = defineProps({
  loading: { type: Boolean, default: false },
  error: { type: String, default: '' },
  onLogin: { type: Function, required: true },
})

const loginEmail = ref('')
const loginPassword = ref('')

const handleLogin = async () => {
  try {
    await props.onLogin(loginEmail.value.trim(), loginPassword.value)
  } catch (err) {}
}
</script>

<template>
  <div class="auth-page">
    <div class="auth-card">
      <h1>Prihlásenie</h1>

      <p class="auth-subtitle">
        Prihlás sa pre prístup k aplikácii
      </p>

      <div class="auth-form">
        <input
          v-model="loginEmail"
          type="email"
          placeholder="Email"
          autocomplete="username"
          maxlength="100"
        />
        <input
          v-model="loginPassword"
          type="password"
          placeholder="Heslo"
          autocomplete="current-password"
          maxlength="128"
        />
        <button class="action" @click="handleLogin" :disabled="loading">
          Prihlásiť
        </button>
      </div>

      <div class="status">
        <span v-if="error" class="error">{{ error }}</span>
        <span v-else-if="loading" class="info">Prosím čakajte...</span>
      </div>
    </div>
  </div>
</template>

<style scoped>
.auth-page {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 30px 20px;
}

.auth-card {
  width: min(420px, 100%);
  padding: 32px;
  border-radius: var(--radius-lg);
  background: var(--bg-card);
  border: 1px solid var(--border-color);
  box-shadow: 0 8px 30px rgba(0, 0, 0, 0.3);
  text-align: center;
}

.auth-card h1 {
  margin: 0 0 8px;
  color: var(--text-primary);
  font-size: 1.6rem;
  font-weight: 600;
}

.auth-subtitle {
  margin: 0 0 24px;
  color: var(--text-secondary);
  font-size: 0.9rem;
}

.auth-form {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.auth-form input {
  padding: 11px 14px;
  border-radius: var(--radius);
  border: 1px solid var(--border-color);
  background: var(--bg-input);
  color: var(--text-primary);
  font-size: 0.95rem;
  transition: border-color 0.2s;
}

.auth-form input:focus {
  outline: none;
  border-color: var(--accent);
}

.auth-form input::placeholder {
  color: var(--text-muted);
}

.action {
  padding: 11px 14px;
  border-radius: var(--radius);
  border: none;
  background: var(--accent);
  color: white;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
}

.action:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.action:hover:not(:disabled) {
  background: var(--accent-hover);
}

.status {
  margin-top: 14px;
  min-height: 24px;
}

.error {
  color: var(--danger);
  font-weight: 500;
  font-size: 0.9rem;
}

@media (max-width: 768px) {
  .auth-page {
    padding: 20px 12px;
  }

  .auth-card {
    padding: 24px 18px;
  }

  .auth-card h1 {
    font-size: 1.35rem;
  }
}

@media (max-width: 480px) {
  .auth-page {
    padding: 16px 8px;
  }

  .auth-card {
    padding: 20px 14px;
  }
}
</style>
