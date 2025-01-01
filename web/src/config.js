// API-Konfiguration
export const API_URL = process.env.VUE_APP_API_URL || 'http://localhost:5000/api'
export const SOCKET_URL = process.env.VUE_APP_SOCKET_URL || 'http://localhost:5000'

// Spielkonfiguration
export const GAME_DURATION = 5 * 60 // 5 Minuten in Sekunden

// Chart.js Standardkonfiguration
export const CHART_COLORS = {
  primary: '#10B981',
  secondary: '#6366F1',
  background: '#1F2937',
  text: '#F9FAFB'
}

// Animationskonfiguration
export const ANIMATION_DURATION = 400 // Millisekunden
