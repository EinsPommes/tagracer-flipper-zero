import { io } from 'socket.io-client'
import { SOCKET_URL } from '@/config'

export default {
  namespaced: true,
  
  state: {
    socket: null,
    connected: false
  },
  
  mutations: {
    SET_SOCKET(state, socket) {
      state.socket = socket
    },
    SET_CONNECTED(state, status) {
      state.connected = status
    }
  },
  
  actions: {
    init({ commit, dispatch }) {
      const socket = io(SOCKET_URL)
      
      socket.on('connect', () => {
        commit('SET_CONNECTED', true)
      })
      
      socket.on('disconnect', () => {
        commit('SET_CONNECTED', false)
      })
      
      socket.on('score_update', (data) => {
        dispatch('game/handleScoreUpdate', data, { root: true })
      })
      
      socket.on('game_over', () => {
        dispatch('game/handleGameOver', null, { root: true })
      })
      
      commit('SET_SOCKET', socket)
    },
    
    disconnect({ state, commit }) {
      if (state.socket) {
        state.socket.disconnect()
        commit('SET_SOCKET', null)
        commit('SET_CONNECTED', false)
      }
    }
  }
}
