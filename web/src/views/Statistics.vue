<template>
  <div class="space-y-6">
    <div class="bg-gray-800 rounded-lg p-6">
      <h1 class="text-2xl font-bold mb-4">Spielstatistiken</h1>
      
      <!-- Statistik-Karten -->
      <div class="grid grid-cols-1 md:grid-cols-3 gap-6 mb-8">
        <div 
          v-for="stat in generalStats" 
          :key="stat.label"
          class="bg-gray-700 rounded-lg p-4"
        >
          <div class="text-gray-400 text-sm">{{ stat.label }}</div>
          <div class="text-2xl font-bold mt-1">{{ stat.value }}</div>
          <div class="text-sm text-gray-400 mt-1">
            <span :class="stat.trend > 0 ? 'text-green-400' : 'text-red-400'">
              {{ stat.trend > 0 ? '↑' : '↓' }} {{ Math.abs(stat.trend) }}%
            </span>
            vs. letzte Woche
          </div>
        </div>
      </div>

      <!-- Aktivitätsgraph -->
      <div class="bg-gray-700 rounded-lg p-4">
        <h2 class="text-lg font-bold mb-4">Aktivität über Zeit</h2>
        <activity-chart 
          :data="activityData"
          class="h-64"
        />
      </div>
    </div>

    <!-- Tag-Statistiken -->
    <div class="bg-gray-800 rounded-lg p-6">
      <h2 class="text-lg font-bold mb-4">Top Tags</h2>
      <div class="overflow-x-auto">
        <table class="min-w-full">
          <thead>
            <tr>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-400 uppercase tracking-wider">
                Tag ID
              </th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-400 uppercase tracking-wider">
                Scans
              </th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-400 uppercase tracking-wider">
                Unique Spieler
              </th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-400 uppercase tracking-wider">
                Punkte
              </th>
              <th class="px-6 py-3 text-left text-xs font-medium text-gray-400 uppercase tracking-wider">
                Letzter Scan
              </th>
            </tr>
          </thead>
          <tbody class="divide-y divide-gray-600">
            <tr 
              v-for="tag in topTags" 
              :key="tag.id"
              class="hover:bg-gray-700"
            >
              <td class="px-6 py-4 whitespace-nowrap">
                <div class="font-medium">{{ tag.id }}</div>
                <div class="text-sm text-gray-400">{{ tag.description }}</div>
              </td>
              <td class="px-6 py-4 whitespace-nowrap">
                {{ tag.scans }}
              </td>
              <td class="px-6 py-4 whitespace-nowrap">
                {{ tag.uniquePlayers }}
              </td>
              <td class="px-6 py-4 whitespace-nowrap">
                {{ tag.points }}
              </td>
              <td class="px-6 py-4 whitespace-nowrap text-gray-400">
                {{ formatDate(tag.lastScan) }}
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- Heatmap -->
    <div class="bg-gray-800 rounded-lg p-6">
      <h2 class="text-lg font-bold mb-4">Tag-Locations</h2>
      <div class="h-96">
        <location-heatmap 
          :locations="tagLocations"
          :center="mapCenter"
        />
      </div>
    </div>
  </div>
</template>

<script>
import { defineComponent } from 'vue'
import { mapState, mapActions } from 'vuex'
import moment from 'moment'
import ActivityChart from '@/components/charts/ActivityChart.vue'
import LocationHeatmap from '@/components/maps/LocationHeatmap.vue'

export default defineComponent({
  name: 'Statistics',
  
  components: {
    ActivityChart,
    LocationHeatmap
  },
  
  data() {
    return {
      mapCenter: { lat: 0, lng: 0 }
    }
  },
  
  computed: {
    ...mapState('stats', [
      'generalStats',
      'activityData',
      'topTags',
      'tagLocations'
    ])
  },
  
  methods: {
    ...mapActions('stats', [
      'fetchStatistics',
      'fetchTagLocations'
    ]),
    
    formatDate(date) {
      return moment(date).fromNow()
    },
    
    async initializeMap() {
      if (this.tagLocations.length > 0) {
        // Berechne den Durchschnitt aller Koordinaten für das Kartenzentrum
        const total = this.tagLocations.reduce(
          (acc, loc) => ({
            lat: acc.lat + loc.lat,
            lng: acc.lng + loc.lng
          }),
          { lat: 0, lng: 0 }
        )
        
        this.mapCenter = {
          lat: total.lat / this.tagLocations.length,
          lng: total.lng / this.tagLocations.length
        }
      }
    }
  },
  
  async created() {
    await Promise.all([
      this.fetchStatistics(),
      this.fetchTagLocations()
    ])
    this.initializeMap()
  }
})
</script>
