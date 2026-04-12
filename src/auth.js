import { ref, computed } from 'vue'
import { auth, db, firebaseConfig } from './firebase'
import { initializeApp, deleteApp } from 'firebase/app'
import {
  createUserWithEmailAndPassword,
  signInWithEmailAndPassword,
  signOut,
  onAuthStateChanged,
  sendPasswordResetEmail,
  getAuth,
} from 'firebase/auth'
import { ref as dbRef, get, update } from 'firebase/database'

const currentUser = ref(null)
const role = ref(null)
const profile = ref({ firstName: '', lastName: '' })
const loading = ref(false)
const error = ref(null)
const authReady = ref(false)

const isLoggedIn = computed(() => !!currentUser.value)
const isAdmin = computed(() => role.value === 'admin')

let _authReadyResolve
const authReadyPromise = new Promise((resolve) => {
  _authReadyResolve = resolve
})

async function loadUserRole(uid) {
  if (!uid) {
    role.value = null
    profile.value = { firstName: '', lastName: '' }
    return
  }

  try {
    const snapshot = await get(dbRef(db, `users/${uid}`))
    if (snapshot.exists()) {
      const data = snapshot.val()
      role.value = data.role || 'user'
      profile.value = {
        firstName: data.firstName || '',
        lastName: data.lastName || '',
      }
    } else {
      role.value = null
      profile.value = { firstName: '', lastName: '' }
    }
  } catch (err) {
    console.error('Failed to load user role', err)
    role.value = null
    profile.value = { firstName: '', lastName: '' }
  }
}

function startAuthListener() {
  onAuthStateChanged(auth, async (firebaseUser) => {
    if (firebaseUser) {
      currentUser.value = { uid: firebaseUser.uid, email: firebaseUser.email }
      await loadUserRole(firebaseUser.uid)
    } else {
      currentUser.value = null
      role.value = null
      profile.value = { firstName: '', lastName: '' }
    }
    if (!authReady.value) {
      authReady.value = true
      _authReadyResolve()
    }
  })
}

function validateEmail(email) {
  if (typeof email !== 'string') return false
  const trimmed = email.trim()
  if (!trimmed) return false
  return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(trimmed)
}

async function login(email, password) {
  loading.value = true
  error.value = null

  if (!validateEmail(email) || !password) {
    error.value = 'Email a heslo musia byť vyplnené.'
    loading.value = false
    return null
  }

  try {
    const credential = await signInWithEmailAndPassword(auth, email.trim(), password)
    const user = credential.user
    currentUser.value = { uid: user.uid, email: user.email }
    await loadUserRole(user.uid)
    return currentUser.value
  } catch (err) {
    console.error('Login error', err)
    if (err.code === 'auth/user-not-found' || err.code === 'auth/invalid-credential') {
      error.value = 'Nesprávny email alebo heslo.'
    } else if (err.code === 'auth/wrong-password') {
      error.value = 'Nesprávny email alebo heslo.'
    } else if (err.code === 'auth/too-many-requests') {
      error.value = 'Príliš veľa pokusov. Skúste neskôr.'
    } else {
      error.value = err?.message || 'Prihlásenie zlyhalo.'
    }
    return null
  } finally {
    loading.value = false
  }
}

async function resetPassword(email) {
  loading.value = true
  error.value = null

  if (!validateEmail(email)) {
    error.value = 'Email musí byť zadaný.'
    loading.value = false
    return null
  }

  try {
    await sendPasswordResetEmail(auth, email.trim())
    return true
  } catch (err) {
    console.error('Reset password error', err)
    if (err.code === 'auth/user-not-found') {
      return true
    } else {
      error.value = err?.message || 'Reset hesla zlyhal.'
    }
    return null
  } finally {
    loading.value = false
  }
}

async function createAdmin(firstName, lastName, email, password) {
  loading.value = true
  error.value = null

  if (!firstName || !lastName || !validateEmail(email) || !password) {
    error.value = 'Všetky polia musia byť vyplnené.'
    loading.value = false
    return null
  }

  if (password.length < 6) {
    error.value = 'Heslo musí mať aspoň 6 znakov.'
    loading.value = false
    return null
  }

  try {
    const tempApp = initializeApp(firebaseConfig, 'temp-create-admin')
    const tempAuth = getAuth(tempApp)
    const credential = await createUserWithEmailAndPassword(tempAuth, email.trim(), password)
    const newUser = credential.user
    await deleteApp(tempApp)

    await update(dbRef(db, `users/${newUser.uid}`), {
      firstName,
      lastName,
      email: newUser.email,
      role: 'admin',
      createdAt: new Date().toISOString(),
    })

    return { uid: newUser.uid, email: newUser.email }
  } catch (err) {
    console.error('Create admin error', err)
    if (err.code === 'auth/email-already-in-use') {
      error.value = 'Tento email už je zaregistrovaný.'
    } else if (err.code === 'auth/weak-password') {
      error.value = 'Heslo je príliš slabé (minimálne 6 znakov).'
    } else if (err.code === 'auth/invalid-email') {
      error.value = 'Neplatný formát emailu.'
    } else {
      error.value = err?.message || 'Vytvorenie admina zlyhalo.'
    }
    return null
  } finally {
    loading.value = false
  }
}

async function deleteUser(uid) {
  loading.value = true
  error.value = null

  try {
    await update(dbRef(db, `users/${uid}`), {
      firstName: null,
      lastName: null,
      email: null,
      role: null,
      createdAt: null,
    })
    return true
  } catch (err) {
    console.error('Delete user error', err)
    error.value = err?.message || 'Odstránenie používateľa zlyhalo.'
    return null
  } finally {
    loading.value = false
  }
}

async function updateProfileData(firstName, lastName) {
  loading.value = true
  error.value = null

  if (!currentUser.value) {
    error.value = 'Nie je prihlásený žiadny používateľ.'
    loading.value = false
    return null
  }

  if (!firstName || !lastName) {
    error.value = 'Meno a priezvisko musia byť vyplnené.'
    loading.value = false
    return null
  }

  try {
    const uid = currentUser.value.uid
    await update(dbRef(db, `users/${uid}`), {
      firstName,
      lastName,
    })
    profile.value = { firstName, lastName }
    return true
  } catch (err) {
    console.error('Failed to update profile', err)
    error.value = err?.message || 'Aktualizácia profilu zlyhala.'
    return null
  } finally {
    loading.value = false
  }
}

async function logout() {
  loading.value = true
  error.value = null

  try {
    await signOut(auth)
    currentUser.value = null
    role.value = null
    profile.value = { firstName: '', lastName: '' }
  } catch (err) {
    console.error('Logout error', err)
    error.value = err?.message || 'Odhlásenie zlyhalo'
    throw err
  } finally {
    loading.value = false
  }
}

export function useAuth() {
  return {
    currentUser,
    role,
    profile,
    isLoggedIn,
    isAdmin,
    loading,
    error,
    authReadyPromise,
    login,
    resetPassword,
    createAdmin,
    deleteUser,
    updateProfileData,
    logout,
    startAuthListener,
  }
}
