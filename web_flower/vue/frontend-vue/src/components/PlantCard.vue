<template>
  <el-card class="plant-card" shadow="hover" @click="goToDetail">
    <div class="header">
      <h2>{{ plant.nickname }}（{{ plant.species }}）</h2>
      <el-tag :type="modeTagColor(plant.mode)">
        当前模式：{{ modeLabel(plant.mode) }}
      </el-tag>
    </div>

    <div class="main-content">
      <!-- 左侧植物图片 -->
      <div class="plant-image">
        <img :src="getImageUrl(plant.image_url)" alt="植物图片" />
      </div>

      <!-- 右侧传感器数据 -->
      <div class="sensor-grid">
        <div>🌡️ 温度：{{ plant.temperature ?? '-' }} ℃</div>
        <div>💧 湿度：{{ plant.humidity ?? '-' }} %</div>
        <div>🌱 土壤湿度：{{ plant.soil_moisture ?? '-' }} %</div>
        <div>☀️ 光照：{{ plant.light ?? '-' }} lx</div>
        <div>⚡ 电量：{{ plant.battery ?? '-' }} %</div>
        <div>🪣 水位：{{ plant.water_level ?? '-' }} %</div>
      </div>
    </div>

    <div class="actions">
      <el-button type="primary" @click.stop="startWatering">一键浇水</el-button>
    </div>
  </el-card>
</template>

<script setup>
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'

const props = defineProps({ plant: Object })
const router = useRouter()

const getImageUrl = (filename) => {
  try {
    return new URL(`../sc/${filename || 'default.png'}`, import.meta.url).href
  } catch {
    return new URL(`../sc/default.png`, import.meta.url).href
  }
}

const modeTagColor = (mode) => {
  switch (mode) {
    case 'watering': return 'primary'
    case 'chat': return 'warning'
    case 'listening': return 'info'
    default: return 'success'
  }
}

const modeLabel = (mode) => {
  switch (mode) {
    case 'watering': return '浇水中'
    case 'chat': return '语音模式'
    case 'listening': return '监听中'
    default: return '默认'
  }
}

const startWatering = () => {
  ElMessage.success(`🌧️ 已为「${props.plant.nickname}」发送浇水命令！`)
  // TODO：你可以在这里调用 sendControlCommand({ plant_id: ... }) 接口
}

const goToDetail = () => {
  router.push(`/status/${props.plant.plant_id}`)
}
</script>

<style scoped>
.plant-card {
  width: 100%;
  padding: 24px;
  border-radius: 16px;
  background: linear-gradient(135deg, #ffffff, #ffffff);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
  transition: all 0.3s ease;
  cursor: pointer;
  border: 1px solid #eaeaea;
}
.plant-card:hover {
  transform: translateY(-4px) scale(1.01);
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.1);
}

/* 标题与模式标签 */
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.header h2 {
  font-size: 20px;
  margin: 0;
  color: #333;
}

/* 主体内容布局 */
.main-content {
  display: flex;
  gap: 24px;
  margin-top: 18px;
  align-items: center;
}

/* 植物图像样式 */
.plant-image img {
  width: 180px;
  height: 180px;
  object-fit: cover;
  border-radius: 12px;
  box-shadow: 0 0 12px rgba(0, 0, 0, 0.08);
  border: 2px solid #e3e6ec;
}

/* 传感器数据布局 */
.sensor-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px 20px;
  font-size: 15px;
  color: #555;
  padding: 12px;
  background: #fcfcfc;
  border-radius: 12px;
}

/* 操作按钮区域 */
.actions {
  margin-top: 24px;
  text-align: center;
}
</style>
