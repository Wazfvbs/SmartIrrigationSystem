<template>
  <el-form>
    <el-form-item label="用户名">
      <el-input v-model="username" />
    </el-form-item>
    <el-form-item label="密码">
      <el-input v-model="password" type="password" />
    </el-form-item>
    <el-button type="primary" @click="handleLogin">登录</el-button>
  </el-form>
</template>

<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { login } from '@/api/auth' // ✅ 引入封装模块

const username = ref('')
const password = ref('')
const router = useRouter()

const handleLogin = async () => {
  try {
    const res = await login({
      username: username.value,
      password: password.value
    })

    const token = res.data.token
    const role = res.data.user.role || 'USER' // 兼容默认角色

    localStorage.setItem('token', token)
    localStorage.setItem('role', role)

    ElMessage.success('登录成功！')
    if (role === 'ADMIN') {
      router.push('/admin/dashboard')
    } else {
      router.push('/home')
    }
  } catch (e) {
    ElMessage.error('登录失败：用户名或密码错误')
  }
}
</script>
