<template>
  <div class="space-y-6">
    <!-- Filter und Zeitraum -->
    <div class="bg-gray-800 rounded-lg p-6">
      <div class="flex flex-wrap gap-4 items-center justify-between">
        <div class="flex gap-4">
          <select 
            v-model="selectedPeriod"
            class="bg-gray-700 border-gray-600 text-white rounded-md"
          >
            <option value="daily">Heute</option>
            <option value="weekly">Diese Woche</option>
            <option value="monthly">Dieser Monat</option>
            <option value="allTime">Gesamt</option>
          </select>
          
          <select 
            v-model="selectedRegion"
            class="bg-gray-700 border-gray-600 text-white rounded-md"
          >
            <option value="global">Global</option>
            <option v-for="region in regions" 
                    :key="region.id" 
                    :value="region.id"
            >
              {{ region.name }}
            </option>
          </select>
        </div>
        
        <button 
          @click="refreshLeaderboard"
          class="bg-blue-600 hover:bg-blue-700 px-4 py-2 rounded-md"
        >
          Aktualisieren
        </button>
      </div>
    </div>

    <!-- Bestenliste -->
    <div class="bg-gray-800 rounded-lg p-6">
      <div class="space-y-4">
        <!-- Top 3 Podium -->
        <div class="flex justify-center items-end gap-4 mb-8">
          <!-- Zweiter Platz -->
          <div class="text-center">
            <div class="w-24 h-24 mx-auto mb-2">
              <img 
                :src="getAvatarUrl(topPlayers[1])"
                class="w-full h-full rounded-full border-4 border-gray-400"
              >
            </div>
            <div class="bg-gray-700 p-4 rounded-lg text-center">
              <div class="text-3xl mb-2">🥈</div>
              <div class="font-medium">{{ topPlayers[1]?.nickname }}</div>
              <div class="text-2xl font-bold">{{ topPlayers[1]?.points }}</div>
            </div>
          </div>

          <!-- Erster Platz -->
          <div class="text-center">
            <div class="w-32 h-32 mx-auto mb-2">
              <img 
                :src="getAvatarUrl(topPlayers[0])"
                class="w-full h-full rounded-full border-4 border-yellow-400"
              >
            </div>
            <div class="bg-gray-700 p-4 rounded-lg text-center">
              <div class="text-3xl mb-2">👑</div>
              <div class="font-medium">{{ topPlayers[0]?.nickname }}</div>
              <div class="text-3xl font-bold">{{ topPlayers[0]?.points }}</div>
            </div>
          </div>

          <!-- Dritter Platz -->
          <div class="text-center">
            <div class="w-24 h-24 mx-auto mb-2">
              <img 
                :src="getAvatarUrl(topPlayers[2])"
                class="w-full h-full rounded-full border-4 border-yellow-700"
              >
            </div>
            <div class="bg-gray-700 p-4 rounded-lg text-center">
              <div class="text-3xl mb-2">🥉</div>
              <div class="font-medium">{{ topPlayers[2]?.nickname }}</div>
              <div class="text-2xl font-bold">{{ topPlayers[2]?.points }}</div>
            </div>
          </div>
        </div>

        <!-- Restliche Spieler -->
        <div class="space-y-2">
          <div 
            v-for="(player, index) in remainingPlayers" 
            :key="player.id"
            class="bg-gray-700 rounded-lg p-4 flex items-center"
          >
            <div class="w-8 text-center font-bold text-gray-400">
              {{ index + 4 }}
            </div>
            <div class="w-12 h-12 mx-4">
              <img 
                :src="getAvatarUrl(player)"
                class="w-full h-full rounded-full"
              >
            </div>
            <div class="flex-grow">
              <div class="font-medium">{{ player.nickname }}</div>
              <div class="text-sm text-gray-400">
                Tags: {{ player.tagCount }} • Spiele: {{ player.gameCount }}
              </div>
            </div>
            <div class="text-2xl font-bold">
              {{ player.points }}
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Achievements -->
    <div class="bg-gray-800 rounded-lg p-6">
      <h2 class="text-xl font-bold mb-4">Achievements</h2>
      <div class="grid grid-cols-2 md:grid-cols-4 gap-4">
        <div 
          v-for="achievement in achievements" 
          :key="achievement.id"
          class="bg-gray-700 rounded-lg p-4 text-center"
          :class="{ 'opacity-50': !achievement.unlocked }"
        >
          <div class="text-3xl mb-2">{{ achievement.icon }}</div>
          <div class="font-medium">{{ achievement.name }}</div>
          <div class="text-sm text-gray-400">{{ achievement.description }}</div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { defineComponent } from 'vue'
import { mapState, mapActions } from 'vuex'

export default defineComponent({
  name: 'Leaderboard',
  
  data() {
    return {
      selectedPeriod: 'weekly',
      selectedRegion: 'global',
      regions: [
        { id: 'eu', name: 'Europa' },
        { id: 'na', name: 'Nordamerika' },
        { id: 'as', name: 'Asien' }
      ]
    }
  },
  
  computed: {
    ...mapState('leaderboard', [
      'players',
      'achievements'
    ]),
    
    topPlayers() {
      return this.players.slice(0, 3)
    },
    
    remainingPlayers() {
      return this.players.slice(3)
    }
  },
  
  methods: {
    ...mapActions('leaderboard', [
      'fetchLeaderboard',
      'fetchAchievements'
    ]),
    
    getAvatarUrl(player) {
      return player?.avatar || 'https://via.placeholder.com/100'
    },
    
    async refreshLeaderboard() {
      await this.fetchLeaderboard({
        period: this.selectedPeriod,
        region: this.selectedRegion
      })
    }
  },
  
  watch: {
    selectedPeriod() {
      this.refreshLeaderboard()
    },
    selectedRegion() {
      this.refreshLeaderboard()
    }
  },
  
  async created() {
    await Promise.all([
      this.refreshLeaderboard(),
      this.fetchAchievements()
    ])
  }
})
</script>
