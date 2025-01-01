<template>
  <div class="fixed bottom-0 right-0 m-6 space-y-4">
    <transition-group name="notification">
      <div
        v-for="notification in notifications"
        :key="notification.id"
        class="bg-gray-800 rounded-lg shadow-lg p-4 max-w-sm"
        :class="{
          'border-l-4 border-green-500': notification.type === 'success',
          'border-l-4 border-red-500': notification.type === 'error',
          'border-l-4 border-blue-500': notification.type === 'info'
        }"
      >
        <div class="flex items-start">
          <div class="flex-shrink-0">
            <CheckCircleIcon
              v-if="notification.type === 'success'"
              class="h-6 w-6 text-green-500"
            />
            <ExclamationCircleIcon
              v-if="notification.type === 'error'"
              class="h-6 w-6 text-red-500"
            />
            <InformationCircleIcon
              v-if="notification.type === 'info'"
              class="h-6 w-6 text-blue-500"
            />
          </div>
          <div class="ml-3 w-0 flex-1">
            <p class="text-sm font-medium text-white">
              {{ notification.title }}
            </p>
            <p class="mt-1 text-sm text-gray-300">
              {{ notification.message }}
            </p>
          </div>
          <div class="ml-4 flex-shrink-0 flex">
            <button
              @click="removeNotification(notification.id)"
              class="rounded-md inline-flex text-gray-400 hover:text-gray-300 focus:outline-none"
            >
              <XIcon class="h-5 w-5" />
            </button>
          </div>
        </div>
      </div>
    </transition-group>
  </div>
</template>

<script>
import { defineComponent } from 'vue'
import {
  CheckCircleIcon,
  ExclamationCircleIcon,
  InformationCircleIcon,
  XIcon
} from '@heroicons/vue/solid'

export default defineComponent({
  name: 'NotificationsOverlay',
  
  components: {
    CheckCircleIcon,
    ExclamationCircleIcon,
    InformationCircleIcon,
    XIcon
  },
  
  data() {
    return {
      notifications: []
    }
  },
  
  methods: {
    addNotification(notification) {
      const id = Date.now()
      this.notifications.push({
        id,
        ...notification
      })
      
      // Automatisch nach 5 Sekunden entfernen
      setTimeout(() => {
        this.removeNotification(id)
      }, 5000)
    },
    
    removeNotification(id) {
      const index = this.notifications.findIndex(n => n.id === id)
      if (index !== -1) {
        this.notifications.splice(index, 1)
      }
    }
  }
})
</script>

<style scoped>
.notification-enter-active,
.notification-leave-active {
  transition: all 0.4s ease;
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
