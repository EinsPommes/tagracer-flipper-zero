<template>
  <div class="relative">
    <canvas ref="chart"></canvas>
  </div>
</template>

<script>
import { defineComponent } from 'vue'
import { Chart, registerables } from 'chart.js'
import { CHART_COLORS } from '@/config'

Chart.register(...registerables)

export default defineComponent({
  name: 'ActivityChart',
  
  props: {
    data: {
      type: Array,
      required: true
    }
  },
  
  data() {
    return {
      chart: null
    }
  },
  
  methods: {
    initChart() {
      const ctx = this.$refs.chart.getContext('2d')
      
      this.chart = new Chart(ctx, {
        type: 'line',
        data: {
          labels: this.data.map(d => d.time),
          datasets: [{
            label: 'Tag Scans',
            data: this.data.map(d => d.scans),
            borderColor: CHART_COLORS.primary,
            backgroundColor: CHART_COLORS.primary + '40',
            fill: true,
            tension: 0.4
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: {
            legend: {
              display: false
            },
            tooltip: {
              mode: 'index',
              intersect: false,
              backgroundColor: CHART_COLORS.background,
              titleColor: CHART_COLORS.text,
              bodyColor: CHART_COLORS.text,
              borderColor: CHART_COLORS.border,
              borderWidth: 1
            }
          },
          scales: {
            y: {
              beginAtZero: true,
              grid: {
                color: CHART_COLORS.background + '20'
              },
              ticks: {
                color: CHART_COLORS.text,
                font: {
                  family: "'Inter', sans-serif"
                }
              }
            },
            x: {
              grid: {
                display: false
              },
              ticks: {
                color: CHART_COLORS.text,
                font: {
                  family: "'Inter', sans-serif"
                },
                maxRotation: 45,
                minRotation: 45
              }
            }
          },
          interaction: {
            intersect: false,
            mode: 'index'
          },
          animation: {
            duration: 750,
            easing: 'easeInOutQuart'
          }
        }
      })
    },
    
    updateChart() {
      if(this.chart) {
        this.chart.data.labels = this.data.map(d => d.time)
        this.chart.data.datasets[0].data = this.data.map(d => d.scans)
        this.chart.update('none')
      }
    }
  },
  
  watch: {
    data: {
      handler() {
        this.updateChart()
      },
      deep: true
    }
  },
  
  mounted() {
    this.initChart()
  },
  
  beforeUnmount() {
    if(this.chart) {
      this.chart.destroy()
    }
  }
})
</script>
