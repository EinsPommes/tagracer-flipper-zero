import { createStore } from 'vuex'
import gameModule from './modules/game'
import socketModule from './modules/socket'
import statsModule from './modules/stats'

export default createStore({
  modules: {
    game: gameModule,
    socket: socketModule,
    stats: statsModule
  }
})
