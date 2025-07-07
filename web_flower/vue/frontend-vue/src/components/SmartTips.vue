<template>
    <el-card class="smart-tips" shadow="hover">
      <div class="header">
        <span>🌟 智能推荐</span>
        <el-button text icon="Refresh" @click="refreshTips" :loading="loading">刷新建议</el-button>
      </div>
      <div class="content" v-if="tips.length > 0">
        <ul>
          <ul v-for="(tip, index) in tips" :key="index">{{ tip }}</ul>
        </ul>
      </div>
      <div class="empty" v-else>
        暂无推荐建议
      </div>
    </el-card>
  </template>
  
  <script setup>
  import { ref, watch } from 'vue'
  
  const props = defineProps({
    data: {
      type: Object,
      required: true
    }
  })
  
  const tips = ref([])
  const loading = ref(false)
  
  const generateTips = (data) => {
    const result = []
  
    // ✅ 土壤湿度建议
    if (data.soil_moisture !== undefined) {
      if (data.soil_moisture < 50) {
        result.push(`🪴 土壤湿度为 ${data.soil_moisture}%，偏干，建议适度浇水`)
      } else if (data.soil_moisture > 80) {
        result.push(`🌧️ 土壤湿度为 ${data.soil_moisture}%，偏高，请注意排水`)
      } else {
        result.push(`🪴 土壤湿度为 ${data.soil_moisture}%，状态良好`)
      }
    }
  
    // ✅ 光照建议
    if (data.light !== undefined) {
      if (data.light < 200) {
        result.push(`🌥️ 当前光照 ${data.light} lx 偏低，建议加强照明`)
      } else if (data.light > 1000) {
        result.push(`🌞 当前光照 ${data.light} lx 较强，注意避免暴晒`)
      } else {
        result.push(`🌞 光照为 ${data.light} lx，环境良好`)
      }
    }
  
    // ✅ 电量建议
    if (data.battery !== undefined) {
      if (data.battery < 20) {
        result.push(`🔋 电量仅 ${data.battery}% ，请及时充电`)
      } else {
        result.push(`⚡ 当前电量 ${data.battery}% ，设备状态正常`)
      }
    }
  
    // ✅ 水位建议
    if (data.water_level !== undefined) {
      if (data.water_level < 30) {
        result.push(`🚿 水箱剩余 ${data.water_level}% ，建议补水`)
      } else {
        result.push(`💧 水位 ${data.water_level}% ，无需补水`)
      }
    }
  
    return result
  }
  
  const refreshTips = () => {
    loading.value = true
    setTimeout(() => {
      tips.value = generateTips(props.data)
      loading.value = false
    }, 300)
  }
  
  // 自动响应数据变化
  watch(() => props.data, refreshTips, { immediate: true })
  </script>
  
  <style scoped>
  .smart-tips {
  border-radius: 12px;
  background: #ffffff;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.05);
  
  }
  .header {
  font-weight: bold;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 18px;
  margin-bottom: 12px;
}
  .content ul {
    padding: 6px 0;
  font-size: 16px;
  }
  .empty {
    color: #999;
    font-style: italic;
  }
  </style>
  