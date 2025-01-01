<template>
  <div class="min-h-screen bg-gray-900 text-white">
    <nav class="bg-gray-800 shadow-lg">
      <div class="max-w-7xl mx-auto px-4">
        <div class="flex justify-between h-16">
          <div class="flex">
            <router-link to="/" class="flex items-center">
              <img class="h-8 w-8" src="@/assets/logo.svg" alt="TagRacer">
              <span class="ml-2 text-xl font-bold">TagRacer</span>
            </router-link>
          </div>
          <div class="flex items-center space-x-4">
            <router-link 
              v-for="item in navItems" 
              :key="item.path" 
              :to="item.path"
              class="px-3 py-2 rounded-md text-sm font-medium hover:bg-gray-700"
              :class="$route.path === item.path ? 'bg-gray-700' : ''"
            >
              {{ item.name }}
            </router-link>
          </div>
        </div>
      </div>
    </nav>

    <main class="max-w-7xl mx-auto px-4 py-6">
      <router-view></router-view>
    </main>

    <notifications-overlay />
  </div>
</template>

<script>
import { defineComponent } from 'vue'
import NotificationsOverlay from '@/components/NotificationsOverlay.vue'

export default defineComponent({
  name: 'App',
  components: {
    NotificationsOverlay
  },
  data() {
    return {
      navItems: [
        { name: 'Live Game', path: '/' },
        { name: 'Leaderboard', path: '/leaderboard' },
        { name: 'Statistics', path: '/statistics' }
      ]
    }
  }
})
</script>
