<template>
  <el-card class="control-panel" shadow="hover">
    <h3 class="panel-title">🛠 控制面板</h3>
    <el-row :gutter="20">
      <el-col :span="6" v-for="action in actions" :key="action.cmd">
        <el-button
          :type="action.type"
          :icon="action.icon"
          class="control-btn"
          :loading="loading"
          @click="sendCommand(action.cmd)"
        >
          {{ action.label }}
        </el-button>
      </el-col>
    </el-row>
  </el-card>
</template>

<script setup>
import { ref } from 'vue'
import { ElMessage } from 'element-plus'
import { sendControlCommand } from '@/api/control'

const props = defineProps({
  plantId: String
})

const loading = ref(false)

// 定义操作项
const actions = [
  { label: '一键浇水', cmd: 'start_watering', type: 'primary', icon: 'el-icon-water-cup' },
  { label: 'AI 模式', cmd: 'enable_ai', type: 'success', icon: 'el-icon-cpu' },
  { label: '停止操作', cmd: 'stop_all', type: 'danger', icon: 'el-icon-close' },
  { label: '强制排水', cmd: 'force_drain', type: 'warning', icon: 'el-icon-delete' },
  { label: '清除日志', cmd: 'clear_log', type: 'info', icon: 'el-icon-document-delete' },
  { label: '维护模式', cmd: 'enter_maintenance', type: 'default', icon: 'el-icon-setting' }
]

// 发送命令方法
const sendCommand = async (cmd) => {
  if (!props.plantId) {
    ElMessage.error('缺少 plant_id')
    return
  }

  loading.value = true
  try {
    const res = await sendControlCommand({ plant_id: props.plantId, cmd })
    if (res.data.code === 200) {
      ElMessage.success(res.data.message || '命令发送成功')
    } else {
      ElMessage.error(res.data.message || '命令失败')
    }
  } catch (e) {
    ElMessage.error('接口异常')
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.control-panel {
  border-radius: 12px;
  background: #ffffff;
  box-shadow: 0 6px 14px rgba(0, 0, 0, 0.05);
  padding: 20px;
}
.panel-title {
  margin-bottom: 16px;
  font-weight: bold;
  font-size: 18px;
}
.control-btn {
  width: 100%;
  height: 42px;
  font-weight: 500;
  border-radius: 8px;
  margin-top: 12px;
}
</style>
