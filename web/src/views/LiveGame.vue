<template>
  <div class="space-y-6">
    <div class="bg-gray-800 rounded-lg p-6">
      <div class="flex justify-between items-center">
        <div>
          <h1 class="text-2xl font-bold">Live Game</h1>
          <p class="text-gray-400">
            {{ gameStatus }}
          </p>
        </div>
        <button 
          v-if="!currentGame"
          @click="startNewGame" 
          class="bg-green-600 hover:bg-green-700 px-4 py-2 rounded-md"
        >
          Start New Game
        </button>
        <div v-else class="text-xl">
          Time Remaining: {{ formatTime(timeRemaining) }}
        </div>
      </div>
    </div>

    <div v-if="currentGame" class="grid grid-cols-1 md:grid-cols-2 gap-6">
      <!-- Live Scoreboard -->
      <div class="bg-gray-800 rounded-lg p-6">
        <h2 class="text-xl font-bold mb-4">Live Scores</h2>
        <div class="space-y-4">
          <div 
            v-for="(score, index) in sortedScores" 
            :key="score.player_id"
            class="bg-gray-700 rounded-lg p-4 flex justify-between items-center"
          >
            <div class="flex items-center">
              <div class="w-8 h-8 bg-gray-600 rounded-full flex items-center justify-center font-bold">
                {{ index + 1 }}
              </div>
              <div class="ml-4">
                <div class="font-medium">{{ score.nickname }}</div>
                <div class="text-sm text-gray-400">Tags: {{ score.tag_count }}</div>
              </div>
            </div>
            <div class="text-2xl font-bold">{{ score.points }}</div>
          </div>
        </div>
      </div>

      <!-- Recent Activity -->
      <div class="bg-gray-800 rounded-lg p-6">
        <h2 class="text-xl font-bold mb-4">Recent Activity</h2>
        <div class="space-y-4">
          <div 
            v-for="activity in recentActivity" 
            :key="activity.id"
            class="bg-gray-700 rounded-lg p-4"
          >
            <div class="flex justify-between">
              <span class="font-medium">{{ activity.player }}</span>
              <span class="text-green-400">+{{ activity.points }}</span>
            </div>
            <div class="text-sm text-gray-400">
              {{ activity.tag_id }}
              <span class="mx-2">•</span>
              {{ formatTime(activity.timestamp) }}
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Game Over Screen -->
    <div 
      v-if="gameOver"
      class="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center"
    >
      <div class="bg-gray-800 rounded-lg p-8 max-w-md w-full mx-4">
        <h2 class="text-2xl font-bold mb-4">Game Over!</h2>
        <div class="space-y-4">
          <div 
            v-for="(score, index) in sortedScores.slice(0, 3)" 
            :key="score.player_id"
            class="flex items-center"
          >
            <div class="text-2xl font-bold mr-4">
              {{ ['🥇', '🥈', '🥉'][index] }}
            </div>
            <div>
              <div class="font-medium">{{ score.nickname }}</div>
              <div class="text-gray-400">{{ score.points }} points</div>
            </div>
          </div>
        </div>
        <button 
          @click="startNewGame"
          class="mt-6 w-full bg-green-600 hover:bg-green-700 px-4 py-2 rounded-md"
        >
          Start New Game
        </button>
      </div>
    </div>
  </div>
</template>

<script>
import { defineComponent } from 'vue'
import { mapState, mapActions } from 'vuex'
import moment from 'moment'

export default defineComponent({
  name: 'LiveGame',
  
  computed: {
    ...mapState({
      currentGame: state => state.game.currentGame,
      scores: state => state.game.scores,
      recentActivity: state => state.game.recentActivity
    }),
    
    gameStatus() {
      if (!this.currentGame) return 'No active game'
      if (this.timeRemaining <= 0) return 'Game Over'
      return 'Game in progress'
    },
    
    timeRemaining() {
      if (!this.currentGame) return 0
      const endTime = moment(this.currentGame.start_time).add(5, 'minutes')
      return Math.max(0, endTime.diff(moment(), 'seconds'))
    },
    
    gameOver() {
      return this.currentGame && this.timeRemaining <= 0
    },
    
    sortedScores() {
      return [...this.scores].sort((a, b) => b.points - a.points)
    }
  },
  
  methods: {
    ...mapActions({
      startNewGame: 'game/startNewGame'
    }),
    
    formatTime(seconds) {
      if (typeof seconds === 'string') return moment(seconds).fromNow()
      const mins = Math.floor(seconds / 60)
      const secs = seconds % 60
      return `${mins}:${secs.toString().padStart(2, '0')}`
    }
  }
})
</script>
