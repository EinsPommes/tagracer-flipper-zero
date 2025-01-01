import axios from 'axios'
import { API_URL } from '@/config'

export default {
  namespaced: true,
  
  state: {
    currentGame: null,
    scores: [],
    recentActivity: [],
    error: null
  },
  
  mutations: {
    SET_CURRENT_GAME(state, game) {
      state.currentGame = game
    },
    SET_SCORES(state, scores) {
      state.scores = scores
    },
    ADD_ACTIVITY(state, activity) {
      state.recentActivity.unshift(activity)
      if (state.recentActivity.length > 10) {
        state.recentActivity.pop()
      }
    },
    UPDATE_SCORE(state, { playerId, points }) {
      const player = state.scores.find(s => s.player_id === playerId)
      if (player) {
        player.points = points
        player.tag_count++
      }
    },
    SET_ERROR(state, error) {
      state.error = error
    },
    CLEAR_GAME(state) {
      state.currentGame = null
      state.scores = []
      state.recentActivity = []
    }
  },
  
  actions: {
    async startNewGame({ commit }) {
      try {
        const response = await axios.post(`${API_URL}/games/start`)
        commit('SET_CURRENT_GAME', response.data)
        return response.data
      } catch (error) {
        commit('SET_ERROR', error.message)
        throw error
      }
    },
    
    async fetchCurrentGame({ commit }) {
      try {
        const response = await axios.get(`${API_URL}/games/current`)
        commit('SET_CURRENT_GAME', response.data)
        commit('SET_SCORES', response.data.scores)
        return response.data
      } catch (error) {
        if (error.response && error.response.status === 404) {
          commit('CLEAR_GAME')
        } else {
          commit('SET_ERROR', error.message)
        }
        throw error
      }
    },
    
    handleScoreUpdate({ commit }, data) {
      commit('UPDATE_SCORE', {
        playerId: data.player_id,
        points: data.points
      })
      commit('ADD_ACTIVITY', {
        id: Date.now(),
        player: data.player_nickname || data.player_id,
        points: data.points,
        tag_id: data.tag_id,
        timestamp: new Date().toISOString()
      })
    },
    
    handleGameOver({ commit }) {
      commit('CLEAR_GAME')
    }
  }
}
