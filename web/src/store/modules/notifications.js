let nextId = 1

const state = {
  notifications: []
}

const mutations = {
  ADD_NOTIFICATION(state, notification) {
    state.notifications.push({
      id: nextId++,
      progress: 100,
      ...notification
    })
  },
  
  REMOVE_NOTIFICATION(state, id) {
    const index = state.notifications.findIndex(n => n.id === id)
    if(index !== -1) {
      state.notifications.splice(index, 1)
    }
  },
  
  UPDATE_PROGRESS(state, { id, progress }) {
    const notification = state.notifications.find(n => n.id === id)
    if(notification) {
      notification.progress = progress
    }
  }
}

const actions = {
  addNotification({ commit, dispatch }, notification) {
    commit('ADD_NOTIFICATION', notification)
    
    if(notification.duration > 0) {
      const id = nextId - 1
      const startTime = Date.now()
      const duration = notification.duration
      
      const updateProgress = () => {
        const elapsed = Date.now() - startTime
        const progress = Math.max(0, 100 - (elapsed / duration * 100))
        
        commit('UPDATE_PROGRESS', { id, progress })
        
        if(progress > 0) {
          requestAnimationFrame(updateProgress)
        } else {
          dispatch('removeNotification', id)
        }
      }
      
      requestAnimationFrame(updateProgress)
    }
  },
  
  removeNotification({ commit }, id) {
    commit('REMOVE_NOTIFICATION', id)
  },
  
  showSuccess({ dispatch }, { title, message, duration = 5000 }) {
    dispatch('addNotification', {
      type: 'success',
      title,
      message,
      duration
    })
  },
  
  showError({ dispatch }, { title, message, duration = 8000 }) {
    dispatch('addNotification', {
      type: 'error',
      title,
      message,
      duration
    })
  },
  
  showWarning({ dispatch }, { title, message, duration = 6000 }) {
    dispatch('addNotification', {
      type: 'warning',
      title,
      message,
      duration
    })
  },
  
  showInfo({ dispatch }, { title, message, duration = 4000 }) {
    dispatch('addNotification', {
      type: 'info',
      title,
      message,
      duration
    })
  }
}

export default {
  namespaced: true,
  state,
  mutations,
  actions
}
