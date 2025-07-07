// src/api/auth.js
import request from '@/utils/request'

/**
 * 用户登录
 * @param {Object} data - 包含 username 和 password 的对象
 * @returns Promise
 */
export function login(data) {
  return request.post('/auth/login', data)
}

/**
 * 用户注册（如需要）
 */
export function register(data) {
  return request.post('/auth/register', data)
}
