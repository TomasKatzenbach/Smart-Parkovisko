<script setup>
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'
import ESPStatusPanel from './ESPStatusPanel.vue'
import { useAuth } from '../auth'
import { db } from '../firebase.js'
import { ref as dbRef, onValue, set, query, orderByChild, limitToLast } from 'firebase/database'

const props = defineProps({
  currentUser: { type: Object, default: null },
  isAdmin: { type: Boolean, default: false },
})

const {
  deleteUser,
  createAdmin,
  loading,
  error,
} = useAuth()

const users = ref([])
const search = ref('')
let usersUnsubscribe = null
const newAdminFirstName = ref('')
const newAdminLastName = ref('')
const newAdminEmail = ref('')
const newAdminPassword = ref('')
const createMessage = ref('')
const creating = ref(false)
const showCreateAdmin = ref(false)

const handleCreateAdmin = async () => {
  creating.value = true
  createMessage.value = ''
  const result = await createAdmin(
    newAdminFirstName.value.trim(),
    newAdminLastName.value.trim(),
    newAdminEmail.value.trim(),
    newAdminPassword.value
  )
  if (result) {
    createMessage.value = `Admin ${result.email} bol úspešne vytvorený.`
    newAdminFirstName.value = ''
    newAdminLastName.value = ''
    newAdminEmail.value = ''
    newAdminPassword.value = ''
    showCreateAdmin.value = false
    setTimeout(() => { createMessage.value = '' }, 5000)
  }
  creating.value = false
}

const filteredUsers = computed(() => {
  if (!search.value) return users.value
  const q = search.value.trim().toLowerCase()
  return users.value.filter((u) => u.email?.toLowerCase().includes(q))
})

const handleDelete = async (uid) => {
  if (props.currentUser?.uid === uid) return
  if (!confirm('Naozaj chceš odstrániť tohto používateľa? Táto akcia sa nedá vrátiť.')) return
  await deleteUser(uid)
}

const sensors = ref([])
const sensorHistory = ref([])
const showSensorHistory = ref(false)
let sensorUnsubscribe = null
let sensorHistoryUnsubscribe = null

const espStatus = ref(null)
const currentTime = ref(Date.now())
let espStatusUnsubscribe = null
let timeRefreshTimer = null

const espOffline = computed(() => {
  const timestamp = Number(espStatus.value?.timestamp || 0)
  if (!timestamp) return true
  const ageSeconds = Math.floor(currentTime.value / 1000) - timestamp
  return ageSeconds > 60
})

const formatTimestamp = (ts) => {
  if (!ts) return '—'
  const d = new Date(ts * 1000)
  return d.toLocaleString('sk-SK')
}

const getSensorStatusClass = (spot) => {
  if (espOffline.value) return 'sensor-offline'
  if (spot.sensorError || spot.state === 'Error') return 'sensor-error'
  if (spot.state === 'Free') return 'sensor-free'
  return 'sensor-occupied'
}

const getSensorStatusText = (spot) => {
  if (espOffline.value) return 'Odpojené'
  if (spot.sensorError || spot.state === 'Error') return 'Chyba'
  if (spot.state === 'Free') return 'Voľné'
  return 'Obsadené'
}

const apRequesting = ref(false)
const apMessage = ref('')

const requestAPMode = async () => {
  apRequesting.value = true
  apMessage.value = ''
  try {
    await set(dbRef(db, 'esp_config/request_ap_mode'), true)
    apMessage.value = 'Požiadavka na AP mód bola odoslaná. ESP sa prepne do AP režimu.'
    setTimeout(() => { apMessage.value = '' }, 5000)
  } catch (err) {
    apMessage.value = 'Chyba: ' + (err.message || 'Nepodarilo sa odoslať požiadavku.')
  } finally {
    apRequesting.value = false
  }
}

onMounted(() => {
  const espStatusRef = dbRef(db, 'esp_config/status')
  espStatusUnsubscribe = onValue(espStatusRef, (snapshot) => {
    espStatus.value = snapshot.exists() ? snapshot.val() : null
  })

  timeRefreshTimer = window.setInterval(() => {
    currentTime.value = Date.now()
  }, 5000)

  const usersRef = dbRef(db, 'users')
  usersUnsubscribe = onValue(usersRef, (snapshot) => {
    if (!snapshot.exists()) {
      users.value = []
      return
    }
    const data = snapshot.val()
    users.value = Object.entries(data).map(([uid, u]) => ({ uid, ...u }))
  })

  const parkingRef = dbRef(db, 'parking_spots')
  sensorUnsubscribe = onValue(parkingRef, (snapshot) => {
    if (!snapshot.exists()) return
    const data = snapshot.val()
    sensors.value = Object.entries(data).map(([id, value]) => ({ id, ...value }))
  })

  const historyQuery = query(dbRef(db, 'sensor_history'), orderByChild('timestamp'), limitToLast(100))
  sensorHistoryUnsubscribe = onValue(historyQuery, (snapshot) => {
    if (!snapshot.exists()) {
      sensorHistory.value = []
      return
    }
    const entries = []
    snapshot.forEach((child) => {
      const d = child.val()
      entries.push({
        id: d.spot,
        type: d.event === 'sensor_error_started' ? 'error' : 'recovery',
        message: d.event === 'sensor_error_started'
          ? `${d.spot} — senzor prešiel do chybového stavu`
          : `${d.spot} — senzor je opäť funkčný`,
        time: d.time || formatTimestamp(d.timestamp),
      })
    })
    sensorHistory.value = entries.reverse()
  })
})

onBeforeUnmount(() => {
  if (usersUnsubscribe) usersUnsubscribe()
  if (sensorUnsubscribe) sensorUnsubscribe()
  if (sensorHistoryUnsubscribe) sensorHistoryUnsubscribe()
  if (espStatusUnsubscribe) espStatusUnsubscribe()
  if (timeRefreshTimer) window.clearInterval(timeRefreshTimer)
})
</script>

<template>
  <div class="admin-dashboard">
    <h1>Admin Dashboard</h1>

    <section class="admin-section">
      <div class="section-header">
        <h2>Správa používateľov</h2>
        <button class="history-button" @click="showCreateAdmin = true">
          Pridať admina
        </button>
      </div>
      <div class="admin-row">
        <input
          v-model="search"
          type="text"
          placeholder="Hľadaj podľa emailu..."
          autocomplete="nope"
          name="dashboard-search"
        />
      </div>
      <div v-if="createMessage" class="status" style="margin-bottom: 12px;">
        <span class="ok">{{ createMessage }}</span>
      </div>

      <div class="table-container">
        <table class="user-table">
          <thead>
            <tr>
              <th>Email</th>
              <th>Meno</th>
              <th>Akcie</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="user in filteredUsers" :key="user.uid">
              <td>{{ user.email }}</td>
              <td>{{ user.firstName }} {{ user.lastName }}</td>
              <td>
                <button
                  class="delete"
                  @click="() => handleDelete(user.uid)"
                  :disabled="user.uid === props.currentUser?.uid"
                >
                  Odstrániť
                </button>
              </td>
            </tr>
          </tbody>
        </table>
      </div>

      <div v-if="showCreateAdmin" class="modal-overlay" @click="showCreateAdmin = false">
        <div class="modal-content" @click.stop>
          <div class="modal-header">
            <h2>Pridať nového admina</h2>
            <button class="close-button" @click="showCreateAdmin = false">✕</button>
          </div>
          <div class="modal-body">
            <div class="form-grid">
              <input
                v-model="newAdminFirstName"
                type="text"
                placeholder="Meno"
                maxlength="50"
                autocomplete="nope"
                name="new-admin-fname"
              />
              <input
                v-model="newAdminLastName"
                type="text"
                placeholder="Priezvisko"
                maxlength="50"
                autocomplete="nope"
                name="new-admin-lname"
              />
              <input
                v-model="newAdminEmail"
                type="email"
                placeholder="Email"
                maxlength="100"
                autocomplete="nope"
                name="new-admin-email"
              />
              <input
                v-model="newAdminPassword"
                type="password"
                placeholder="Heslo (min. 6 znakov)"
                maxlength="128"
                autocomplete="new-password"
                name="new-admin-pass"
              />
            </div>
            <button
              class="create-button"
              @click="handleCreateAdmin"
              :disabled="creating || loading"
            >
              {{ creating ? 'Vytváranie...' : 'Vytvoriť admina' }}
            </button>
            <div class="status">
              <span v-if="error" class="error">{{ error }}</span>
            </div>
          </div>
        </div>
      </div>
    </section>

    <section class="admin-section">
      <div class="section-header">
        <h2>Stav senzorov</h2>
        <button class="history-button" @click="showSensorHistory = true">
          História
        </button>
      </div>
      <div class="sensor-grid">
        <div
          v-for="spot in sensors"
          :key="spot.id"
          class="sensor-card"
          :class="getSensorStatusClass(spot)"
        >
          <div class="sensor-header">
            <span class="sensor-name">{{ spot.id }}</span>
            <span class="sensor-pill" :class="getSensorStatusClass(spot)">
              {{ getSensorStatusText(spot) }}
            </span>
          </div>
          <div class="sensor-details">
            <div class="sensor-detail">
              <span class="detail-label">Vzdialenosť</span>
              <span class="detail-value">{{ spot.distance === -1 ? '—' : spot.distance + ' cm' }}</span>
            </div>
            <div class="sensor-detail">
              <span class="detail-label">Aktualizované</span>
              <span class="detail-value">{{ formatTimestamp(spot.timestamp) }}</span>
            </div>
          </div>
        </div>
      </div>

      <div v-if="showSensorHistory" class="modal-overlay" @click="showSensorHistory = false">
        <div class="modal-content" @click.stop>
          <div class="modal-header">
            <h2>História chýb senzorov</h2>
            <button class="close-button" @click="showSensorHistory = false">✕</button>
          </div>

          <div v-if="sensorHistory.length === 0" class="empty-state">
            Žiadne záznamy v histórii
          </div>

          <div v-else class="sensor-history">
            <div
              v-for="(entry, i) in sensorHistory"
              :key="i"
              class="history-entry"
              :class="entry.type"
            >
              <span class="history-dot" :class="entry.type"></span>
              <div class="history-content">
                <span class="history-message">{{ entry.message }}</span>
                <span class="history-time">{{ entry.time }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>

    <section class="admin-section">
      <h2>ESP Monitoring</h2>
      <ESPStatusPanel />

      <div class="ap-mode-section">
        <p class="ap-description">
          Ak potrebuješ zmeniť WiFi nastavenia ESP, zapni AP mód. ESP vytvorí vlastný hotspot,
          na ktorý sa pripojíš a nakonfiguruješ novú WiFi cez lokálny portál (192.168.4.1).
        </p>
        <button
          class="ap-button"
          :disabled="apRequesting"
          @click="requestAPMode"
        >
          {{ apRequesting ? 'Odosielam...' : 'Zapnúť AP mód' }}
        </button>
        <div v-if="apMessage" class="ap-message">{{ apMessage }}</div>
      </div>
    </section>
  </div>
</template>

<style scoped>
.admin-dashboard {
  padding: 30px 20px;
  max-width: 1000px;
  margin: 0 auto;
  color: var(--text-primary);
}

.admin-dashboard h1 {
  margin: 0 0 18px;
  font-size: 1.8rem;
  font-weight: 600;
}

.admin-section {
  margin-bottom: 24px;
  background: var(--bg-card);
  padding: 24px;
  border-radius: var(--radius-lg);
  border: 1px solid var(--border-color);
}

.admin-section h2 {
  margin: 0 0 16px;
  font-size: 1.15rem;
  font-weight: 600;
  color: var(--text-primary);
}

.admin-row {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
  margin-bottom: 14px;
}

.admin-row input {
  min-width: 200px;
  padding: 10px 12px;
  border-radius: var(--radius);
  border: 1px solid var(--border-color);
  background: var(--bg-input);
  color: var(--text-primary);
  font-size: 0.9rem;
  transition: border-color 0.2s;
}

.admin-row input:focus {
  outline: none;
  border-color: var(--accent);
}

.admin-row input::placeholder {
  color: var(--text-muted);
}

.table-container {
  width: 100%;
  overflow-x: auto;
  margin-top: 12px;
  border-radius: 8px;
}

.user-table {
  width: 100%;
  min-width: 600px;
  border-collapse: collapse;
}

.user-table th,
.user-table td {
  padding: 10px 12px;
  border-bottom: 1px solid var(--border-color);
  text-align: left;
  font-size: 0.9rem;
}

.user-table th {
  background: rgba(255, 255, 255, 0.03);
  color: var(--text-secondary);
  font-weight: 600;
  text-transform: uppercase;
  font-size: 0.75rem;
  letter-spacing: 0.05em;
}

.user-table td {
  color: var(--text-primary);
}

.modal-body {
  padding: 18px 22px 22px;
}

.modal-body .form-grid {
  margin-bottom: 14px;
}

.modal-body .create-button {
  width: 100%;
  padding: 12px;
}

.modal-body .status {
  margin-top: 12px;
}

.delete {
  padding: 6px 12px;
  border-radius: 6px;
  border: none;
  background: var(--danger-muted);
  color: var(--danger);
  font-weight: 500;
  font-size: 0.85rem;
  cursor: pointer;
  transition: all 0.2s;
}

.delete:hover:not(:disabled) {
  background: rgba(239, 68, 68, 0.2);
}

.delete:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
  margin-bottom: 14px;
}

.form-grid input {
  padding: 10px 12px;
  border-radius: var(--radius);
  border: 1px solid var(--border-color);
  background: var(--bg-input);
  color: var(--text-primary);
  font-size: 0.9rem;
  transition: border-color 0.2s;
}

.form-grid input:focus {
  outline: none;
  border-color: var(--accent);
}

.form-grid input::placeholder {
  color: var(--text-muted);
}

.create-button {
  padding: 10px 14px;
  border-radius: var(--radius);
  border: none;
  background: var(--accent);
  color: white;
  font-weight: 600;
  font-size: 0.9rem;
  cursor: pointer;
  transition: all 0.2s ease;
}

.create-button:hover:not(:disabled) {
  background: var(--accent-hover);
}

.create-button:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.status {
  margin-top: 12px;
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

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.section-header h2 {
  margin: 0;
}

.history-button {
  padding: 6px 14px;
  border-radius: 8px;
  border: 1px solid var(--border-color);
  background: transparent;
  color: var(--text-secondary);
  font-weight: 500;
  font-size: 0.85rem;
  cursor: pointer;
  transition: all 0.2s ease;
  white-space: nowrap;
}

.history-button:hover {
  background: rgba(255, 255, 255, 0.04);
  border-color: var(--border-color-hover);
  color: var(--text-primary);
}

.sensor-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
  gap: 12px;
}

.sensor-card {
  background: var(--bg-input);
  border: 1px solid var(--border-color);
  border-radius: var(--radius);
  padding: 16px;
  transition: border-color 0.2s;
}

.sensor-card.sensor-error {
  border-color: #6b7280;
}

.sensor-card.sensor-offline {
  border-color: var(--danger);
}

.sensor-card.sensor-free {
  border-color: var(--success);
}

.sensor-card.sensor-occupied {
  border-color: var(--danger);
}

.sensor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.sensor-name {
  font-weight: 600;
  font-size: 0.95rem;
  color: var(--text-primary);
}

.sensor-pill {
  padding: 3px 10px;
  border-radius: 20px;
  font-size: 0.75rem;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.03em;
}

.sensor-pill.sensor-free {
  background: rgba(34, 197, 94, 0.15);
  color: var(--success);
}

.sensor-pill.sensor-occupied {
  background: rgba(239, 68, 68, 0.15);
  color: var(--danger);
}

.sensor-pill.sensor-error {
  background: rgba(107, 114, 128, 0.15);
  color: #9ca3af;
}

.sensor-pill.sensor-offline {
  background: rgba(239, 68, 68, 0.15);
  color: var(--danger);
}

.sensor-details {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.sensor-detail {
  display: flex;
  justify-content: space-between;
  font-size: 0.82rem;
}

.detail-label {
  color: var(--text-muted);
}

.detail-value {
  color: var(--text-secondary);
  font-weight: 500;
}

.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.6);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  padding: 20px;
  backdrop-filter: blur(4px);
}

.modal-content {
  background: var(--bg-card);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  max-width: 600px;
  width: 100%;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.5);
  animation: slideUp 0.3s ease;
}

@keyframes slideUp {
  from { transform: translateY(30px); opacity: 0; }
  to { transform: translateY(0); opacity: 1; }
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 18px 22px;
  border-bottom: 1px solid var(--border-color);
}

.modal-header h2 {
  margin: 0;
  font-size: 1.15rem;
  font-weight: 600;
  color: var(--text-primary);
}

.close-button {
  background: rgba(255, 255, 255, 0.06);
  border: 1px solid var(--border-color);
  color: var(--text-secondary);
  width: 32px;
  height: 32px;
  border-radius: 8px;
  font-size: 1.1rem;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.2s ease;
}

.close-button:hover {
  background: rgba(255, 255, 255, 0.1);
  color: var(--text-primary);
}

.empty-state {
  padding: 32px 22px;
  text-align: center;
  color: var(--text-muted);
  font-size: 0.9rem;
}

.sensor-history {
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding: 16px 22px;
  overflow-y: auto;
  max-height: calc(80vh - 70px);
}

.history-entry {
  display: flex;
  align-items: flex-start;
  gap: 10px;
  padding: 10px 12px;
  border-radius: var(--radius);
  background: var(--bg-input);
  border: 1px solid var(--border-color);
}

.history-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  margin-top: 4px;
  flex-shrink: 0;
}

.history-dot.error {
  background: #6b7280;
}

.history-dot.recovery {
  background: var(--success);
}

.history-content {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 0;
}

.history-message {
  font-size: 0.88rem;
  color: var(--text-primary);
  font-weight: 500;
}

.history-time {
  font-size: 0.78rem;
  color: var(--text-muted);
}

@media (max-width: 768px) {
  .admin-dashboard {
    padding: 20px 15px;
  }

  .admin-dashboard h1 {
    font-size: 1.5rem;
  }

  .admin-section {
    padding: 16px;
    margin-bottom: 20px;
  }

  .admin-row input {
    min-width: 100%;
    width: 100%;
  }

  .user-table {
    font-size: 0.85rem;
    min-width: 500px;
  }

  .user-table th,
  .user-table td {
    padding: 8px 6px;
  }

  .delete {
    padding: 4px 8px;
    font-size: 0.85rem;
  }

  .form-grid {
    grid-template-columns: 1fr;
  }

  .sensor-grid {
    grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
  }

  .create-button {
    width: 100%;
    padding: 12px;
  }
}

@media (max-width: 480px) {
  .admin-dashboard h1 {
    font-size: 1.3rem;
  }

  .admin-section {
    padding: 12px;
  }

  .user-table th,
  .user-table td {
    padding: 6px 4px;
    font-size: 0.8rem;
  }
}

.ap-mode-section {
  margin-top: 20px;
  padding-top: 18px;
  border-top: 1px solid var(--border-color);
}

.ap-description {
  color: var(--text-secondary);
  font-size: 0.88rem;
  line-height: 1.5;
  margin-bottom: 14px;
}

.ap-button {
  padding: 10px 20px;
  border-radius: var(--radius);
  border: none;
  background: #f59e0b;
  color: #1a1a1a;
  font-weight: 600;
  font-size: 0.9rem;
  cursor: pointer;
  transition: all 0.2s ease;
}

.ap-button:hover:not(:disabled) {
  background: #d97706;
}

.ap-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.ap-message {
  margin-top: 10px;
  padding: 10px 14px;
  border-radius: var(--radius);
  background: rgba(245, 158, 11, 0.1);
  border: 1px solid rgba(245, 158, 11, 0.25);
  color: #fbbf24;
  font-size: 0.85rem;
  font-weight: 500;
}
</style>
