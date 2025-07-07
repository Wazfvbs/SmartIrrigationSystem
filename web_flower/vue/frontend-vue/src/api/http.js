import axios from 'axios'

const instance = axios.create({
  baseURL: '/api',
  timeout: 5000,
  withCredentials: true
})

// 请求拦截器，自动添加 token
instance.interceptors.request.use(config => {
  const token = localStorage.getItem('token')
  if (token) {
    config.headers.Authorization = 'Bearer ' + token
  }
  return config
})

export default instance
