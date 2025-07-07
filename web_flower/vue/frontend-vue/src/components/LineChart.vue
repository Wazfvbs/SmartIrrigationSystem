<template>
  <div ref="chartRef" class="chart" />
</template>

<script setup>
import { onMounted, onUnmounted, ref, watch } from 'vue'
import * as echarts from 'echarts'

const props = defineProps({
  title: String,
  xData: Array,
  yData: Array,
  yLabel: String
})

const chartRef = ref(null)
let chartInstance = null

// 初始化图表
const initChart = () => {
  if (chartRef.value) {
    chartInstance = echarts.init(chartRef.value)
    updateChart()
  }
}

// 更新图表数据
const updateChart = () => {
  if (!chartInstance) return
  chartInstance.setOption({
    title: {
      text: props.title
    },
    tooltip: {
      trigger: 'axis'
    },
    xAxis: {
      type: 'category',
      data: props.xData,
      boundaryGap: false
    },
    yAxis: {
      type: 'value',
      name: props.yLabel
    },
    series: [
      {
        name: props.title,
        type: 'line',
        smooth: true,
        data: props.yData,
        areaStyle: {}
      }
    ],
    toolbox: {
      feature: {
        saveAsImage: {},
        dataZoom: { yAxisIndex: 'none' }
      }
    },
    dataZoom: [
      { type: 'inside' },
      { type: 'slider' }
    ]
  })
}

// 监听数据变化：深度监听数组内容
watch(
  () => [props.xData, props.yData, props.title],
  updateChart,
  { deep: true }
)

onMounted(initChart)
onUnmounted(() => {
  chartInstance?.dispose()
})
</script>

<style scoped>
.chart {
  width: 100%;
  height: 400px;
}
</style>
