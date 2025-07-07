<template>
    
  <el-card class="weather-card" shadow="hover">
    <div class="header">
        <span>🌈 当前天气</span>
      </div>
    <div class="weather-line">城市：{{ weather.city }}</div>
    <div class="weather-line">天气：{{ weather.condition }}</div>
    <div class="weather-line">温度：{{ weather.temperature }} ℃</div>
    <div class="weather-line">湿度：{{ weather.humidity }} %，风速：{{ weather.wind_speed }} km/h</div>
    <div class="weather-empty" ></div>
  </el-card>
</template>

<script setup>
const props = defineProps({
  weather: {
    type: Object,
    default: () => ({ city: '', condition: '', temperature: 0, humidity: 0, wind_speed: 0 })
  }
})
</script>

<style scoped>
.header {
  margin-top: 6px;
  font-weight: bold;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 18px;
  margin-bottom: 24px;
}
.weather-card {
  border-radius: 12px;
  background: #ffffff;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.05);
}
.card-title {
  margin-bottom: 12px;
  font-weight: bold;
}
.weather-line {
  padding: 6px 0;
  font-size: 16px;
}
.weather-empty {
  margin-bottom: 24px;
}
</style>
