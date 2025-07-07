<template>
  <div class="container">
    <el-page-header content="历史数据趋势图" @back="goBack" />

    <el-form :inline="true" class="toolbar">
      <el-form-item label="时间范围">
        <el-date-picker
          v-model="dateRange"
          type="datetimerange"
          range-separator="至"
          start-placeholder="开始时间"
          end-placeholder="结束时间"
          :shortcuts="shortcuts"
        />
      </el-form-item>

      <el-form-item label="数据类型">
        <el-select v-model="selectedType" placeholder="请选择">
          <el-option
            v-for="item in dataOptions"
            :key="item.key"
            :label="item.label"
            :value="item.key"
          />
        </el-select>
      </el-form-item>

      <el-form-item>
        <el-button type="primary" @click="loadData" :loading="loading">查询</el-button>
      </el-form-item>
    </el-form>

    <el-empty v-if="!historyData.length && !loading" description="暂无数据" />
    
    <LineChart
      v-else
      :title="typeLabel"
      :xData="xAxisData"
      :yData="yAxisData"
      :yLabel="typeLabel"
    />
  </div>
</template>

<script setup>
import { ref, computed, onMounted, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { fetchPlantHistory } from '../api/plant'
import LineChart from '../components/LineChart.vue'
import dayjs from 'dayjs'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const plantId = route.params.plant_id

const dateRange = ref([dayjs().subtract(1, 'day').toDate(), new Date()])
const selectedType = ref('temperature') // ✅ 原 dataType 改名统一
const loading = ref(false)
const historyData = ref([]) // ✅ 统一用于存储接口返回数据

const dataOptions = [
  { key: 'temperature', label: '温度 (℃)', unit: '℃' },
  { key: 'humidity', label: '湿度 (%)', unit: '%' },
  { key: 'soil_moisture', label: '土壤湿度 (%)', unit: '%' },
  { key: 'light', label: '光照 (lx)', unit: 'lx' },
  { key: 'water_level', label: '水位 (%)', unit: '%' }
]

const typeLabel = computed(() => {
  return dataOptions.find(o => o.key === selectedType.value)?.label || selectedType.value
})

const xAxisData = computed(() => {
  return historyData.value.map(item => dayjs(item.timestamp).format('MM-DD HH:mm'))
})

const yAxisData = computed(() => {
  return historyData.value.map(item => item[selectedType.value] ?? 0)
})

const shortcuts = [
  {
    text: '最近1小时',
    value: () => [dayjs().subtract(1, 'hour').toDate(), new Date()]
  },
  {
    text: '今天',
    value: () => [dayjs().startOf('day').toDate(), new Date()]
  },
  {
    text: '最近7天',
    value: () => [dayjs().subtract(7, 'day').toDate(), new Date()]
  }
]

const loadData = async () => {
  loading.value = true
  historyData.value = []

  const [start, end] = dateRange.value.map(d => dayjs(d).toISOString())

  try {
    const res = await fetchPlantHistory(plantId, start, end)
    if (res.data.code === 200) {
      historyData.value = res.data.data
    } else {
      ElMessage.error(res.data.message || '查询失败')
    }
  } catch (err) {
    ElMessage.error('接口错误')
  } finally {
    loading.value = false
  }
}

// ✅ 监听切换类型时也刷新图表
watch(selectedType, () => {
  if (historyData.value.length > 0) {
    // 会自动触发 computed 更新
  }
})

const goBack = () => router.push('/dashboard')
onMounted(() => {
  loadData()
})
</script>

<style scoped>
.container {
  padding: 20px;
}
.toolbar {
  margin-top: 10px;
  margin-bottom: 20px;
}
</style>
