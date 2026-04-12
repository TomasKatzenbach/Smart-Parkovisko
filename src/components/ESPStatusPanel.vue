<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { ref as dbRef, onValue, query, orderByChild, limitToLast } from 'firebase/database'
import { db } from '../firebase.js'

const espStatus = ref(null)
const currentTime = ref(Date.now())
const showHistory = ref(false)
const historyRecords = ref([])
const historyLoading = ref(false)

let unsubscribeStatus = null
let unsubscribeHistory = null
let refreshTimer = null

const heartbeatAgeSeconds = computed(() => {
  const timestamp = Number(espStatus.value?.timestamp || 0)
  if (!timestamp) return null
  return Math.max(0, Math.floor(currentTime.value / 1000) - timestamp)
})

const heartbeatState = computed(() => {
  if (heartbeatAgeSeconds.value === null) return 'offline'
  if (heartbeatAgeSeconds.value <= 25) return 'online'
  if (heartbeatAgeSeconds.value <= 60) return 'stale'
  return 'offline'
})

const internetStatus = computed(() => {
  if (!espStatus.value) return 'Bez dát'
  if (espStatus.value.apMode) return 'AP mód'
  if (heartbeatState.value === 'online') return 'Pripojené na internet'
  if (heartbeatState.value === 'stale') return 'Heartbeat mešká'
  return 'Odpojené'
})

const heartbeatLabel = computed(() => {
  if (heartbeatAgeSeconds.value === null) return 'Bez heartbeat dát'

  if (heartbeatAgeSeconds.value < 60) {
    return `pred ${heartbeatAgeSeconds.value} s`
  }

  const minutes = Math.floor(heartbeatAgeSeconds.value / 60)
  if (minutes < 60) {
    return `pred ${minutes} min`
  }

  const hours = Math.floor(minutes / 60)
  return `pred ${hours} h`
})

const statusRows = computed(() => {
  if (!espStatus.value) {
    return [
      { label: 'Internet', value: internetStatus.value },
      { label: 'IP adresa', value: '—' },
      { label: 'Heartbeat', value: heartbeatLabel.value },
      { label: 'WiFi', value: '—' },
      { label: 'Stav ESP', value: 'Čakám na prvé dáta z Firebase' },
    ]
  }

  return [
    { label: 'Internet', value: internetStatus.value },
    { label: 'IP adresa', value: espStatus.value.ip || '—' },
    { label: 'Heartbeat', value: heartbeatLabel.value },
    { label: 'WiFi', value: espStatus.value.connectedTo || '—' },
    { label: 'Stav ESP', value: espStatus.value.state || '—' },
    { label: 'Posledné videnie', value: espStatus.value.lastSeen || '—' },
    { label: 'Správa', value: espStatus.value.message || '—' },
  ]
})

const openHistory = () => {
  showHistory.value = true
  historyLoading.value = true
  
  const historyRef = query(
    dbRef(db, 'esp_config/history'),
    orderByChild('timestamp'),
    limitToLast(50)
  )

  unsubscribeHistory = onValue(historyRef, (snapshot) => {
    if (snapshot.exists()) {
      const data = snapshot.val()
      const records = Object.entries(data)
        .map(([key, value]) => ({
          id: key,
          ...value
        }))
        .reverse()
      historyRecords.value = records
    } else {
      historyRecords.value = []
    }
    historyLoading.value = false
  })
}

onMounted(() => {
  const statusRef = dbRef(db, 'esp_config/status')

  unsubscribeStatus = onValue(statusRef, (snapshot) => {
    espStatus.value = snapshot.exists() ? snapshot.val() : null
  })

  refreshTimer = window.setInterval(() => {
    currentTime.value = Date.now()
  }, 1000)
})

onBeforeUnmount(() => {
  if (unsubscribeStatus) unsubscribeStatus()
  if (unsubscribeHistory) unsubscribeHistory()
  if (refreshTimer) window.clearInterval(refreshTimer)
})
</script>

<template>
  <div class="esp-status-panel">
    <div class="panel-header">
      <div>
        <p class="eyebrow">Monitoring zariadenia</p>
        <h3>ESP stav</h3>
      </div>
      <div class="header-controls">
        <div class="status-pill" :class="heartbeatState">
          <span class="status-dot"></span>
          {{ internetStatus }}
        </div>
        <button class="history-button" @click="openHistory">
          História
        </button>
      </div>
    </div>

    <div class="status-grid">
      <div
        v-for="row in statusRows"
        :key="row.label"
        class="status-item"
      >
        <span class="status-label">{{ row.label }}</span>
        <strong class="status-value">{{ row.value }}</strong>
      </div>
    </div>

    <div v-if="showHistory" class="modal-overlay" @click="showHistory = false">
      <div class="modal-content" @click.stop>
        <div class="modal-header">
          <h2>História ESP</h2>
          <button class="close-button" @click="showHistory = false">✕</button>
        </div>

        <div v-if="historyLoading" class="loading">
          Načítavam históriu...
        </div>

        <div v-else-if="historyRecords.length === 0" class="empty-state">
          Žiadne záznamy v histórii
        </div>

        <div v-else class="history-list">
          <div
            v-for="record in historyRecords"
            :key="record.id"
            class="history-item"
          >
            <div class="history-header">
              <span class="history-type" :class="`type-${record.type}`">
                {{ record.type }}
              </span>
              <span class="history-time">{{ record.time }}</span>
            </div>
            <p class="history-message">{{ record.message }}</p>
            <div v-if="record.requestedSSID || record.resultSSID" class="history-details">
              <div v-if="record.requestedSSID" class="detail-row">
                <span class="detail-label">Požiadaná WiFi:</span>
                <span class="detail-value">{{ record.requestedSSID }}</span>
              </div>
              <div v-if="record.resultSSID" class="detail-row">
                <span class="detail-label">Výsledná WiFi:</span>
                <span class="detail-value">{{ record.resultSSID }}</span>
              </div>
            </div>
            <div class="history-footer">
              <span class="detail-label">IP:</span>
              <span class="detail-value">{{ record.ip || '—' }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.esp-status-panel {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
  flex-wrap: wrap;
}

.header-controls {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}

.eyebrow {
  margin: 0 0 4px;
  text-transform: uppercase;
  letter-spacing: 0.06em;
  font-size: 0.72rem;
  color: var(--text-muted);
}

.panel-header h3 {
  margin: 0;
  font-size: 1.25rem;
  font-weight: 600;
  color: var(--text-primary);
}

.status-pill {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 6px 12px;
  border-radius: 999px;
  font-weight: 600;
  font-size: 0.82rem;
  border: 1px solid transparent;
}

.status-pill.online {
  background: var(--success-muted);
  color: var(--success);
  border-color: rgba(34, 197, 94, 0.2);
}

.status-pill.stale {
  background: rgba(245, 158, 11, 0.1);
  color: var(--warning);
  border-color: rgba(245, 158, 11, 0.2);
}

.status-pill.offline {
  background: var(--danger-muted);
  color: var(--danger);
  border-color: rgba(239, 68, 68, 0.2);
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: currentColor;
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

.status-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.status-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 14px;
  border-radius: var(--radius);
  background: rgba(255, 255, 255, 0.02);
  border: 1px solid var(--border-color);
}

.status-label {
  font-size: 0.72rem;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  color: var(--text-muted);
}

.status-value {
  color: var(--text-primary);
  font-size: 0.9rem;
  font-weight: 500;
  line-height: 1.4;
  word-break: break-word;
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
  max-width: 700px;
  width: 100%;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.5);
  animation: slideUp 0.3s ease;
}

@keyframes slideUp {
  from {
    transform: translateY(30px);
    opacity: 0;
  }
  to {
    transform: translateY(0);
    opacity: 1;
  }
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
  font-size: 1.2rem;
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

.history-list {
  flex: 1;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding: 18px 22px;
}

.history-item {
  padding: 12px 14px;
  border-radius: var(--radius);
  background: rgba(255, 255, 255, 0.02);
  border: 1px solid var(--border-color);
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.history-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

.history-type {
  display: inline-block;
  padding: 3px 8px;
  border-radius: 4px;
  font-size: 0.72rem;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.04em;
}

.history-type.type-wifi_switch_success {
  background: var(--success-muted);
  color: var(--success);
}

.history-type.type-wifi_switch_failed {
  background: var(--danger-muted);
  color: var(--danger);
}

.history-type.type-connection {
  background: var(--accent-muted);
  color: var(--accent);
}

.history-time {
  font-size: 0.78rem;
  color: var(--text-muted);
}

.history-message {
  margin: 0;
  color: var(--text-primary);
  font-size: 0.88rem;
  line-height: 1.4;
}

.history-details {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding-top: 6px;
  border-top: 1px solid var(--border-color);
}

.detail-row {
  display: flex;
  gap: 12px;
  font-size: 0.82rem;
}

.detail-label {
  color: var(--text-muted);
  min-width: 120px;
  font-weight: 500;
}

.detail-value {
  color: var(--text-secondary);
  word-break: break-word;
  flex: 1;
}

.history-footer {
  display: flex;
  gap: 12px;
  font-size: 0.78rem;
  padding-top: 6px;
  border-top: 1px solid var(--border-color);
  color: var(--text-muted);
}

.loading,
.empty-state {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
  color: var(--text-muted);
  font-size: 0.95rem;
}

@media (max-width: 720px) {
  .status-grid {
    grid-template-columns: 1fr;
  }

  .panel-header h3 {
    font-size: 1.1rem;
  }

  .header-controls {
    width: 100%;
    flex-direction: column;
  }

  .status-pill,
  .history-button {
    width: 100%;
    justify-content: center;
  }

  .modal-content {
    max-height: 90vh;
  }

  .history-details {
    flex-wrap: wrap;
  }

  .detail-row {
    flex-direction: column;
    gap: 2px;
  }

  .detail-label {
    min-width: auto;
  }
}
</style>