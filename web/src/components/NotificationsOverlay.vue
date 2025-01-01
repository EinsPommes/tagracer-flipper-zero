<template>
  <div 
    class="fixed top-0 right-0 p-4 space-y-4 z-50 max-w-md w-full"
    :class="{ 'pointer-events-none': notifications.length === 0 }"
  >
    <transition-group name="notification">
      <div
        v-for="notification in notifications"
        :key="notification.id"
        class="bg-gray-800 border border-gray-700 rounded-lg shadow-lg p-4 transform transition-all duration-300"
        :class="{
          'border-green-500': notification.type === 'success',
          'border-red-500': notification.type === 'error',
          'border-yellow-500': notification.type === 'warning',
          'border-blue-500': notification.type === 'info'
        }"
      >
        <div class="flex items-start">
          <!-- Icon -->
          <div class="flex-shrink-0">
            <div 
              class="w-8 h-8 rounded-full flex items-center justify-center"
              :class="{
                'bg-green-500': notification.type === 'success',
                'bg-red-500': notification.type === 'error',
                'bg-yellow-500': notification.type === 'warning',
                'bg-blue-500': notification.type === 'info'
              }"
            >
              <i 
                class="text-white text-xl"
                :class="{
                  'fas fa-check': notification.type === 'success',
                  'fas fa-times': notification.type === 'error',
                  'fas fa-exclamation': notification.type === 'warning',
                  'fas fa-info': notification.type === 'info'
                }"
              ></i>
            </div>
          </div>
          
          <!-- Content -->
          <div class="ml-3 w-full">
            <h3 class="text-white font-medium">{{ notification.title }}</h3>
            <p class="mt-1 text-gray-300">{{ notification.message }}</p>
            
            <!-- Progress bar -->
            <div 
              class="mt-2 w-full bg-gray-700 rounded-full h-1 overflow-hidden"
              v-if="notification.duration > 0"
            >
              <div
                class="h-full transition-all duration-100"
                :class="{
                  'bg-green-500': notification.type === 'success',
                  'bg-red-500': notification.type === 'error',
                  'bg-yellow-500': notification.type === 'warning',
                  'bg-blue-500': notification.type === 'info'
                }"
                :style="{ width: `${notification.progress}%` }"
              ></div>
            </div>
          </div>
          
          <!-- Close button -->
          <button
            @click="removeNotification(notification.id)"
            class="ml-4 text-gray-400 hover:text-white transition-colors"
          >
            <i class="fas fa-times"></i>
          </button>
        </div>
      </div>
    </transition-group>
  </div>
</template>

<script>
import { defineComponent } from 'vue'
import { mapState, mapActions } from 'vuex'

export default defineComponent({
  name: 'NotificationsOverlay',
  
  computed: {
    ...mapState('notifications', ['notifications'])
  },
  
  methods: {
    ...mapActions('notifications', ['removeNotification'])
  }
})
</script>

<style scoped>
.notification-enter-active,
.notification-leave-active {
  transition: all 0.3s ease;
}

.notification-enter-from {
  opacity: 0;
  transform: translateX(30px);
}

.notification-leave-to {
  opacity: 0;
  transform: translateX(30px);
}
</style>
