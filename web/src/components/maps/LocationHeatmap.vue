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
      heatLayer: null,
      tileLayer: null
    }
  },
  
  methods: {
    initMap() {
      // Map initialisieren
      this.map = L.map(this.$refs.map).setView(
        [this.center.lat, this.center.lng],
        13
      )
      
      // Dark Mode Tile Layer
      this.tileLayer = L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', {
        attribution: '© OpenStreetMap contributors, © CARTO',
        subdomains: 'abcd',
        maxZoom: 19
      }).addTo(this.map)
      
      // Heatmap initialisieren
      this.updateHeatmap()
      
      // Responsive handling
      window.addEventListener('resize', this.handleResize)
    },
    
    updateHeatmap() {
      if(this.heatLayer) {
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
    },
    
    handleResize() {
      this.map.invalidateSize()
    },
    
    fitBounds() {
      if(this.locations.length > 0) {
        const bounds = L.latLngBounds(this.locations.map(loc => [loc.lat, loc.lng]))
        this.map.fitBounds(bounds, { padding: [50, 50] })
      }
    }
  },
  
  watch: {
    locations: {
      handler() {
        this.updateHeatmap()
        this.fitBounds()
      },
      deep: true
    },
    center: {
      handler(newCenter) {
        if(this.map) {
          this.map.setView([newCenter.lat, newCenter.lng])
        }
      },
      deep: true
    }
  },
  
  mounted() {
    this.$nextTick(() => {
      this.initMap()
    })
  },
  
  beforeUnmount() {
    if(this.map) {
      window.removeEventListener('resize', this.handleResize)
      this.map.remove()
    }
  }
})
</script>

<style>
.leaflet-container {
  background-color: #1F2937;
}

.leaflet-tile {
  filter: brightness(0.6) invert(1) contrast(3) hue-rotate(200deg) saturate(0.3) brightness(0.7);
}

.leaflet-control-attribution {
  background-color: rgba(31, 41, 55, 0.8) !important;
  color: #9CA3AF !important;
}

.leaflet-control-attribution a {
  color: #60A5FA !important;
}
</style>
