<template>
  <h2>欢迎使用智慧浇水系统</h2>
  <el-button type="primary" @click="getUserInfo">获取用户信息</el-button>
  <p>{{ info }}</p>
</template>

<script setup>
import axios from '../api/http'
import { ref } from 'vue'

const info = ref('')

const getUserInfo = async () => {
  try {
    const res = await axios.get('/user/info')
    info.value = res.data
  } catch (e) {
    info.value = '获取失败，可能未登录或 token 失效'
  }
}
</script>
