// src/utils/request.js
import axios from 'axios'

const service = axios.create({
  baseURL: '/api', // 自动代理到 Spring Boot 服务器
  timeout: 5000
})

service.interceptors.request.use(config => {
  const token = localStorage.getItem('token')
  if (token) {
    config.headers['Authorization'] = `Bearer ${token}`
  }
  return config
})

export default service
