<script setup>
import { ref, onMounted, onBeforeUnmount, computed } from 'vue'
import { db } from '../firebase.js'
import { ref as dbRef, onValue } from 'firebase/database'

const parkingData = ref([])
const loading = ref(true)
const error = ref(null)
let unsubscribe = null
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

const freeSpots = computed(() => {
  return parkingData.value.filter(spot => spot.state === 'Free').length
})

const totalSpots = computed(() => {
  return parkingData.value.length
})

const occupiedSpots = computed(() => {
  return totalSpots.value - freeSpots.value
})

const navigationUrl = computed(() => {
  const lat = 48.30889
  const lng = 18.09253
  return `https://www.google.com/maps/dir/?api=1&destination=${lat},${lng}&travelmode=driving`
})

const orderedSpots = computed(() => {
  return ['spot1', 'spot2', 'spot3', 'spot4', 'spot5', 'spot6', 'spot7', 'spot8']
    .map(id => parkingData.value.find(s => s.id === id))
    .filter(Boolean)
})

const spotPositions = {
  spot1: { top: '35%', left: '17%' },
  spot2: { top: '45%', left: '17%' },
  spot3: { top: '55%', left: '17%' },
  spot4: { top: '65%', left: '17%' },
  spot5: { top: '35%', left: '82%' },
  spot6: { top: '45%', left: '82%' },
  spot7: { top: '55%', left: '82%' },
  spot8: { top: '65%', left: '82%' },
}

const getSpotClass = (state) => {
  if (espOffline.value) return 'spot-error'
  if (state === 'Error') return 'spot-error'
  return state === 'Free' ? 'spot-free' : 'spot-occupied'
}

const getSpotNumber = (id) => id.replace('spot', '')
const getSpotStyle = (id) => spotPositions[id] || {}

onMounted(() => {
  const espStatusRef = dbRef(db, 'esp_config/status')
  espStatusUnsubscribe = onValue(espStatusRef, (snapshot) => {
    espStatus.value = snapshot.exists() ? snapshot.val() : null
  }, () => {
    espStatus.value = null
  })

  timeRefreshTimer = window.setInterval(() => {
    currentTime.value = Date.now()
  }, 5000)

  const parkingRef = dbRef(db, 'parking_spots')
  
  unsubscribe = onValue(
    parkingRef,
    (snapshot) => {
      try {
        if (snapshot.exists()) {
          const data = snapshot.val()
          if (typeof data === 'object' && !Array.isArray(data)) {
            parkingData.value = Object.entries(data).map(([key, value]) => ({
              id: key,
              ...value
            }))
          } else {
            parkingData.value = Array.isArray(data) ? data : [data]
          }
        } else {
          parkingData.value = []
        }
        error.value = null
      } catch (err) {
        error.value = err.message
      } finally {
        loading.value = false
      }
    },
    (err) => {
      error.value = err.message
      loading.value = false
    }
  )
})

onBeforeUnmount(() => {
  if (unsubscribe) unsubscribe()
  if (espStatusUnsubscribe) espStatusUnsubscribe()
  if (timeRefreshTimer) window.clearInterval(timeRefreshTimer)
})
</script>

<template>
  <div class="parking-container">
    
    <div v-if="loading" class="loading">
      Loading data...
    </div>
    
    <div v-else-if="error" class="error">
      Chyba: {{ error }}
    </div>
    
    <div v-if="!loading && !error" class="navigate-wrapper">
      <a
        :href="navigationUrl"
        target="_blank"
        rel="noopener noreferrer"
        class="navigate-button"
      >
        Naviguj
      </a>
    </div>

    <div v-if="totalSpots > 0 && !loading && !error" class="availability">
      <div class="availability-label">Dostupnosť</div>
      <div class="availability-bar">
        <div class="availability-fill" :style="{ width: espOffline ? '0%' : (freeSpots / totalSpots * 100) + '%' }"></div>
      </div>
      <div v-if="espOffline" class="availability-warning">Dáta nemusia byť aktuálne</div>
      <div v-else class="availability-percentage">{{ Math.round(freeSpots / totalSpots * 100) }}%</div>
    </div>

    <div v-if="!loading && !error && orderedSpots.length" class="map-section">
      <div class="parking-map">
        <img
          src="/parking-lot.png"
          alt="Mapa parkoviska"
          class="parking-map-image"
        />
        <div class="parking-overlay">
          <div
            v-for="spot in orderedSpots"
            :key="spot.id"
            class="parking-spot"
            :class="getSpotClass(spot.state)"
            :style="getSpotStyle(spot.id)"
            :title="`${spot.id}: ${spot.state}`"
          >
            <div class="spot-number">{{ getSpotNumber(spot.id) }}</div>
            <div class="spot-status">{{ espOffline || spot.state === 'Error' ? 'N/A' : (spot.state === 'Free' ? '✓' : '✗') }}</div>
          </div>
        </div>
      </div>

      <div class="legend">
        <div class="legend-item">
          <div class="legend-dot spot-free"></div>
          <span>Voľné</span>
        </div>
        <div class="legend-item">
          <div class="legend-dot spot-occupied"></div>
          <span>Obsadené</span>
        </div>
        <div class="legend-item">
          <div class="legend-dot spot-error"></div>
          <span>Neznáme</span>
        </div>
      </div>
    </div>

    <div v-if="!loading && !error" class="stats-container">
      <div class="stat-card">
        <div class="stat-number free">{{ espOffline ? '?' : freeSpots }}</div>
        <div class="stat-label">Voľných miest</div>
      </div>

      <div class="stat-card">
        <div class="stat-number occupied">{{ espOffline ? '?' : occupiedSpots }}</div>
        <div class="stat-label">Obsadených miest</div>
      </div>

      <div class="stat-card">
        <div class="stat-number total">{{ totalSpots }}</div>
        <div class="stat-label">Celkovo miest</div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.parking-container {
  max-width: 900px;
  margin: 0 auto;
  padding: 40px 20px;
}

.loading, .error {
  text-align: center;
  padding: 2rem;
  font-size: clamp(0.9rem, 3vw, 1rem);
  border-radius: var(--radius);
  color: var(--text-primary);
}

.error {
  background-color: var(--danger-muted);
  color: var(--danger);
  border: 1px solid rgba(239, 68, 68, 0.2);
}

.stats-container {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16px;
  margin-top: 1.5rem;
  margin-bottom: 2rem;
}

.stat-card {
  background-color: var(--bg-card);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  padding: 28px 16px;
  text-align: center;
  transition: all 0.2s ease;
}

.stat-card:hover {
  border-color: var(--border-color-hover);
}

.stat-number {
  font-size: clamp(1.8rem, 8vw, 3rem);
  font-weight: 700;
  margin-bottom: 6px;
}

.stat-number.free {
  color: var(--success);
}

.stat-number.occupied {
  color: var(--danger);
}

.stat-number.total {
  color: var(--accent);
}

.stat-label {
  color: var(--text-secondary);
  font-size: clamp(0.8rem, 2vw, 0.95rem);
  font-weight: 500;
}

.availability {
  background-color: var(--bg-card);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  padding: 24px;
  max-width: 600px;
  margin: 0 auto 1.5rem auto;
}

.availability-label {
  color: var(--text-primary);
  font-size: clamp(0.9rem, 2vw, 1.05rem);
  font-weight: 600;
  margin-bottom: 12px;
  text-align: center;
}

.availability-bar {
  width: 100%;
  height: 8px;
  background-color: rgba(255, 255, 255, 0.06);
  border-radius: 4px;
  overflow: hidden;
  margin-bottom: 12px;
}

.availability-fill {
  height: 100%;
  background: var(--success);
  transition: width 0.5s ease;
  border-radius: 4px;
}

.availability-percentage {
  color: var(--text-primary);
  font-size: clamp(1.1rem, 3vw, 1.5rem);
  font-weight: 700;
  text-align: center;
}

.availability-warning {
  color: #9ca3af;
  font-size: clamp(0.85rem, 2.5vw, 0.95rem);
  font-weight: 500;
  text-align: center;
}

.navigate-wrapper {
  text-align: center;
  max-width: 600px;
  margin: 0 auto 1.5rem auto;
}

.navigate-button {
  display: block;
  width: 100%;
  padding: 16px;
  font-size: clamp(1rem, 3vw, 1.15rem);
  font-weight: 600;
  color: white;
  background: var(--accent);
  border: none;
  border-radius: var(--radius);
  text-decoration: none;
  cursor: pointer;
  transition: all 0.2s ease;
  text-align: center;
  box-sizing: border-box;
}

.navigate-button:hover {
  background: var(--accent-hover);
}

@media (max-width: 768px) {
  .parking-container {
    padding: 20px 10px;
  }

  .stats-container {
    grid-template-columns: 1fr;
    gap: 12px;
    margin-bottom: 1.5rem;
  }

  .stat-card {
    padding: 20px 15px;
  }

  .availability {
    max-width: 100%;
    padding: 20px 15px;
  }

  .navigate-wrapper {
    max-width: 100%;
  }

  .navigate-button {
    padding: 14px;
  }
}

@media (max-width: 480px) {
  .stat-card {
    padding: 16px 12px;
  }

  .stat-number {
    font-size: clamp(1.5rem, 8vw, 2.2rem);
  }

  .stat-label {
    font-size: 0.85rem;
  }

  .navigate-button {
    padding: 14px;
  }

  .availability {
    padding: 16px 12px;
  }
}

.offline-banner {
  text-align: center;
  padding: 12px 16px;
  background: rgba(107, 114, 128, 0.15);
  border: 1px solid rgba(107, 114, 128, 0.3);
  border-radius: var(--radius);
  color: #9ca3af;
  font-size: 0.9rem;
  font-weight: 500;
  margin-bottom: 1.5rem;
  max-width: 600px;
  margin-left: auto;
  margin-right: auto;
}

.map-section {
  display: flex;
  flex-direction: column;
  gap: 20px;
  margin-top: 1.5rem;
}

.parking-map {
  position: relative;
  width: 100%;
  max-width: 650px;
  margin: 0 auto;
}

.parking-map-image {
  display: block;
  width: 100%;
  height: auto;
  border-radius: var(--radius-lg);
  border: 1px solid var(--border-color);
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
}

.parking-overlay {
  position: absolute;
  inset: 0;
}

.parking-spot {
  position: absolute;
  width: 18%;
  height: 8%;
  border-radius: 6px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 10px;
  box-sizing: border-box;
  transform: translate(-50%, -50%);
  transition: transform 0.2s ease;
  overflow: hidden;
  cursor: pointer;
}

.parking-spot:hover {
  transform: translate(-50%, -50%) scale(1.05);
}

.spot-free {
  background: var(--success);
  box-shadow: 0 2px 8px rgba(34, 197, 94, 0.3);
}

.spot-occupied {
  background: var(--danger);
  box-shadow: 0 2px 8px rgba(239, 68, 68, 0.3);
}

.spot-error {
  background: #6b7280;
  box-shadow: 0 2px 8px rgba(107, 114, 128, 0.3);
}

.spot-number,
.spot-status {
  position: relative;
  z-index: 1;
  color: white;
}

.spot-number {
  font-size: clamp(0.7rem, 1.8vw, 1.1rem);
  font-weight: 700;
}

.spot-status {
  font-size: clamp(0.8rem, 2vw, 1.3rem);
  font-weight: 900;
}

.legend {
  display: flex;
  justify-content: center;
  gap: 30px;
  flex-wrap: wrap;
  background-color: var(--bg-card);
  padding: 14px 20px;
  border-radius: var(--radius);
  border: 1px solid var(--border-color);
  max-width: 350px;
  margin: 0 auto;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 8px;
  color: var(--text-secondary);
  font-size: clamp(0.82rem, 2vw, 0.9rem);
}

.legend-dot {
  width: 20px;
  height: 20px;
  border-radius: 4px;
}

.legend-dot.spot-free {
  background-color: var(--success);
}

.legend-dot.spot-occupied {
  background-color: var(--danger);
}

.legend-dot.spot-error {
  background-color: #6b7280;
}

@media (max-width: 520px) {
  .parking-spot {
    width: 20%;
    height: 9%;
    padding: 4px 6px;
    border-radius: 4px;
  }

  .spot-number {
    font-size: clamp(0.55rem, 2vw, 0.85rem);
  }

  .spot-status {
    font-size: clamp(0.6rem, 2.4vw, 0.95rem);
  }

  .legend {
    gap: 15px;
    padding: 12px;
  }

  .legend-dot {
    width: 16px;
    height: 16px;
  }
}
</style>
