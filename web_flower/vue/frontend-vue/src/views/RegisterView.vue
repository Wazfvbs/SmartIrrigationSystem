<template>
    <el-form :model="form" label-width="80px" class="register-form">
        <el-form-item label="用户名">
            <el-input v-model="form.username" />
        </el-form-item>

        <el-form-item label="密码">
            <el-input v-model="form.password" type="password" />
        </el-form-item>

        <el-form-item label="昵称">
            <el-input v-model="form.nickname" />
        </el-form-item>

        <el-form-item>
            <el-button type="primary" @click="register">注册</el-button>
            <el-button @click="goToLogin">返回登录</el-button>
        </el-form-item>
    </el-form>
</template>

<script setup>
import { ref } from 'vue'
import axios from '../api/http'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'

const router = useRouter()
const form = ref({
    username: '',
    password: '',
    nickname: ''
})

const register = async () => {
    try {
        await axios.post('/auth/register', form.value)
        ElMessage.success('注册成功，请登录')
        router.push('/login')
    } catch (err) {
        ElMessage.error('注册失败，用户名可能已存在')
    }
}

const goToLogin = () => {
    router.push('/login')
}
</script>

<style scoped>
.register-form {
    max-width: 400px;
    margin: 100px auto;
}
</style>
  
