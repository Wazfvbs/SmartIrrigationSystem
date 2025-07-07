<template>
  <div class="container">
    <el-page-header content="植物状态面板" @back="goBack" />
    <div class="mode-row">
      <el-tag :type="modeTagColor(plantData.mode)">
        当前模式：{{ modeLabel(plantData.mode) }}
      </el-tag>
      <el-button type="primary" @click="loadData" :loading="loading">刷新数据</el-button>
    </div>

    <el-row :gutter="20" class="status-row">
      <el-col :span="6" v-for="item in sensorItems" :key="item.key">
        <SensorCard
          :title="item.label"
          :value="plantData[item.key]"
          :unit="item.unit"
          :timestamp="plantData.timestamp"
        />
      </el-col>
    </el-row>
    
    <div class="top-section">
      <el-row :gutter="20">
        <el-col :span="12">
          <SmartTips :data="plantData" />
        </el-col>
        <el-col :span="12">
          <WeatherCard :weather="{
  city: '哈尔滨',
  condition: '多云转晴',
  temperature: 25,
  humidity: 60,
  wind_speed: 12
}" />

        </el-col>
      </el-row>
    </div>

    

    <div class="chart-section">
      <HistoryChart
        :plant-id="plantId"
        :default-type="'temperature'"
        enable-range-picker
        enable-type-select
        enable-drag-bar
      />
    </div>

    <ControlPanel :plant-id="plantId" />
  </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { fetchLatestPlantData } from '../api/plant'
import { ElMessage } from 'element-plus'

import SensorCard from '../components/SensorCard.vue'
import ControlPanel from '../components/ControlPanel.vue'
import SmartTips from '../components/SmartTips.vue'
import WeatherCard from '../components/WeatherCard.vue'
import HistoryChart from '../components/HistoryChart.vue'

const route = useRoute()
const router = useRouter()
const plantId = route.params.plant_id

const plantData = ref({})
const loading = ref(false)
let timer = null

const sensorItems = [
  { key: 'temperature', label: '温度', unit: '℃' },
  { key: 'humidity', label: '湿度', unit: '%' },
  { key: 'soil_moisture', label: '土壤湿度', unit: '%' },
  { key: 'light', label: '光照', unit: 'lx' },
  { key: 'water_level', label: '水位', unit: '%' },
  { key: 'battery', label: '电量', unit: '%' }
]

const loadData = async () => {
  loading.value = true
  try {
    const res = await fetchLatestPlantData(plantId)
    if (res.data.code === 200) {
      plantData.value = res.data.data
    } else {
      ElMessage.error(res.data.message || '获取失败')
    }
  } catch (err) {
    ElMessage.error('接口异常')
  } finally {
    loading.value = false
  }
}

const goBack = () => router.push('/home')

const modeTagColor = (mode) => {
  switch (mode) {
    case 'watering': return 'warning'
    case 'chat': return 'info'
    case 'listening': return 'primary'
    default: return 'success'
  }
}
const modeLabel = (mode) => {
  switch (mode) {
    case 'watering': return '浇水中'
    case 'chat': return '语音交互'
    case 'listening': return '监听中'
    default: return '默认模式'
  }
}

onMounted(() => {
  loadData()
  timer = setInterval(loadData, 30000) // 自动刷新
})

onBeforeUnmount(() => clearInterval(timer))
</script>

<style scoped>
.container {
  padding: 24px;
  background-color: #f8f9fa;
}

.top-section {
  margin-bottom: 20px;
}

.mode-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin: 10px 0 20px;
}

.status-row {
  margin-bottom: 30px;
}

.chart-section {
  margin-bottom: 40px;
}
</style>
