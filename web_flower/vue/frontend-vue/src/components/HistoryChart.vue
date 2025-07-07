<template>
    <div class="chart-container">
      <!-- 工具栏 -->
      <el-form :inline="true" class="toolbar">
        <el-form-item label="时间范围">
          <el-date-picker
            v-model="dateRange"
            type="datetimerange"
            range-separator="至"
            start-placeholder="开始时间"
            end-placeholder="结束时间"
            :shortcuts="shortcuts"
            :default-time="['00:00:00', '23:59:59']"
          />
        </el-form-item>
  
        <el-form-item label="数据类型">
  <el-select
    v-model="selectedType"
    placeholder="请选择数据类型"
    style="width: 220px"
    :value-key="'key'"  
  >
    <template v-for="item in dataOptions" :key="item.key">
      <el-option :label="item.label" :value="item.key">
        <span class="option-content" :style="{ color: item.color }">
          <span class="icon">{{ item.icon }}</span>
          {{ item.label }}
        </span>
      </el-option>
    </template>
  </el-select>
</el-form-item>


  
        <el-form-item>
          <el-button type="primary" @click="loadData" :loading="loading">
            查询 {{ dataOptions.find(o => o.key === selectedType.value)?.label || '' }}

          </el-button>
        </el-form-item>
      </el-form>
  
      <!-- 图表显示 -->
      <el-empty v-if="!historyData.length && !loading" description="暂无历史数据" />
      <v-chart v-else :option="chartOption" autoresize class="echart" />
    </div>
  </template>
  
  <script setup>
  import { ref, computed, watch } from 'vue'
  import * as echarts from 'echarts'
  import { use } from 'echarts/core'
  import {
    TitleComponent, TooltipComponent, GridComponent, LegendComponent
  } from 'echarts/components'
  import { LineChart } from 'echarts/charts'
  import { CanvasRenderer } from 'echarts/renderers'
  import VChart from 'vue-echarts'
  import dayjs from 'dayjs'
  import { ElMessage } from 'element-plus'
  import { fetchPlantHistory } from '@/api/plant'
  import { useRoute } from 'vue-router'
  
  use([TitleComponent, TooltipComponent, GridComponent, LegendComponent, LineChart, CanvasRenderer])
  
  const route = useRoute()
  const plantId = route.params.plant_id
  
  const dateRange = ref([dayjs().subtract(1, 'day').toDate(), new Date()])
  const selectedType = ref('temperature')
  const historyData = ref([])
  const loading = ref(false)
  
  const dataOptions = [
  { key: 'temperature', label: '温度 (℃)', unit: '℃', icon: '🌡️', color: '#e74c3c' },
  { key: 'humidity', label: '湿度 (%)', unit: '%', icon: '💧', color: '#3498db' },
  { key: 'soil_moisture', label: '土壤湿度 (%)', unit: '%', icon: '🌱', color: '#27ae60' },
  { key: 'light', label: '光照 (lx)', unit: 'lx', icon: '☀️', color: '#f39c12' },
  { key: 'water_level', label: '水位 (%)', unit: '%', icon: '🪣', color: '#9b59b6' }
]




  
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
  
  const xAxisData = computed(() =>
    historyData.value.map(item => dayjs(item.timestamp).format('MM-DD HH:mm'))
  )
  
  const yAxisData = computed(() =>
    historyData.value.map(item => item[selectedType.value] ?? 0)
  )
  
  const chartOption = computed(() => ({
    tooltip: { trigger: 'axis' },
    grid: { left: '5%', right: '5%', bottom: '18%', top: '12%' },
    xAxis: {
      type: 'category',
      boundaryGap: false,
      data: xAxisData.value,
      axisLabel: { rotate: 45 }
    },
    yAxis: {
      type: 'value',
      name: dataOptions.find(o => o.key === selectedType.value)?.unit || '',
      nameTextStyle: { align: 'right', padding: [0, 0, 0, 10] }
    },
    series: [
      {
        name: selectedType.value,
        type: 'line',
        data: yAxisData.value,
        smooth: true,
        showSymbol: false,
        areaStyle: {}
      }
    ],
    dataZoom: [
      { type: 'inside', start: 0, end: 100 }, // ✅ 拖动条：鼠标滚动/触摸
      { type: 'slider', bottom: 0, height: 30 } // ✅ 底部滑块
    ]
  }))
  
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
    } catch {
      ElMessage.error('历史数据接口错误')
    } finally {
      loading.value = false
    }
  }
  
  watch(selectedType, () => {
    // 图表自动更新
  })
  </script>
  
  <style scoped>
  .option-content {
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: 500;
}

.option-content .icon {
  font-size: 18px;
}

  .chart-container {
    margin-top: 30px;
    padding: 20px;
    background: #f9fafc;
    border-radius: 12px;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
  }
  .toolbar {
    margin-bottom: 20px;
  }
  .echart {
    width: 100%;
    height: 400px;
  }
  </style>
  