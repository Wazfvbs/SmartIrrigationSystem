<!-- src/views/Error403.vue -->
<template>
    <div class="error-page">
      <h1>403 - 无权限访问</h1>
      <p>您没有权限访问该页面。</p>
      <el-button type="primary" @click="goBack">返回首页</el-button>
    </div>
  </template>
  
  <script setup>
  import { useRouter } from 'vue-router'
  const router = useRouter()
  const goBack = () => {
    router.push('/dashboard')
  }
  </script>
  
  <style scoped>
  .error-page {
    text-align: center;
    margin-top: 100px;
  }
  h1 {
    font-size: 48px;
    color: #f56c6c;
  }
  </style>
  