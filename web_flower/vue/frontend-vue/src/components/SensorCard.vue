<template>
    <el-card class="sensor-card" shadow="hover">
        <div class="sensor-title">{{ title }}</div>
        <div class="sensor-value">{{ value }} <span v-if="unit">{{ unit }}</span></div>
        <div class="sensor-sub" v-if="timestamp">更新时间：{{ timestamp }}</div>
    </el-card>
</template>

<script setup>
defineProps({
    title: String,
    value: [String, Number],
    unit: String,
    timestamp: String
})
</script>

<style scoped>
.sensor-card {
    text-align: center;
}

.sensor-title {
    font-weight: bold;
    margin-bottom: 8px;
}

.sensor-value {
    font-size: 24px;
    margin-bottom: 4px;
}

.sensor-sub {
    font-size: 12px;
    color: #999;
}
</style>
  
