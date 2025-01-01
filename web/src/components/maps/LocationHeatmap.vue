<template>
  <div ref="map" class="w-full h-full rounded-lg overflow-hidden"></div>
</template>

<script>
import { defineComponent } from 'vue'
import 'leaflet/dist/leaflet.css'
import L from 'leaflet'
import 'leaflet.heat'

export default defineComponent({
  name: 'LocationHeatmap',
  
  props: {
    locations: {
      type: Array,
      required: true
    },
    center: {
      type: Object,
      required: true
    }
  },
  
  data() {
    return {
      map: null,
      heatLayer: null
    }
  },
  
  methods: {
    initMap() {
      this.map = L.map(this.$refs.map).setView(
        [this.center.lat, this.center.lng],
        13
      )
      
      L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '© OpenStreetMap contributors'
      }).addTo(this.map)
      
      this.updateHeatmap()
    },
    
    updateHeatmap() {
      if (this.heatLayer) {
        this.map.removeLayer(this.heatLayer)
      }
      
      const points = this.locations.map(loc => [
        loc.lat,
        loc.lng,
        loc.intensity || 1
      ])
      
      this.heatLayer = L.heatLayer(points, {
        radius: 25,
        blur: 15,
        maxZoom: 10,
        gradient: {
          0.4: '#10B981',
          0.6: '#6366F1',
          0.8: '#EC4899',
          1.0: '#EF4444'
        }
      }).addTo(this.map)
    }
  },
  
  watch: {
    locations: {
      handler() {
        this.updateHeatmap()
      },
      deep: true
    },
    center: {
      handler(newCenter) {
        if (this.map) {
          this.map.setView([newCenter.lat, newCenter.lng])
        }
      },
      deep: true
    }
  },
  
  mounted() {
    this.initMap()
  },
  
  beforeUnmount() {
    if (this.map) {
      this.map.remove()
    }
  }
})
</script>

<style>
.leaflet-container {
  background-color: #1F2937;
}
</style>
