import { createRouter, createWebHistory } from 'vue-router'
import LoginView from '../views/LoginView.vue'
import DashboardView from '../views/DashboardView.vue'
import PlantStatusView from '../views/PlantStatusView.vue'
import HistoryChartView from '../views/HistoryChartView.vue'

// 路由配置
const routes = [
  { path: '/', redirect: '/login' },
  { path: '/login', component: LoginView },
  { path: '/dashboard', component: DashboardView },
  { path: '/status/:plant_id', name: 'PlantStatus',component: PlantStatusView },

  { path: '/history/:plant_id', component: HistoryChartView },
  {
    path: '/admin/users',
    component: () => import('@/views/admin/UserManagement.vue'),
    meta: { requiresAuth: true, requiresAdmin: true }
  },
  {
    path: '/admin/dashboard',
    component: () => import('@/views/admin/DashboardView.vue'),
    meta: { requiresAuth: true, requiresAdmin: true }
  },  
  {
    path: '/403',
    component: () => import('@/views/Error403.vue')  // 自定义未授权页面
  },
  {
    path: '/home',
    name: 'Home',
    component: () => import('@/views/Home.vue')
  }
]

// 创建路由实例
const router = createRouter({
  history: createWebHistory(),
  routes
})

// 路由守卫：鉴权处理
router.beforeEach((to, from, next) => {
  const token = localStorage.getItem('token')
  const role = localStorage.getItem('role')

  // 需要登录但没有 token
  if (to.meta.requiresAuth && !token) {
    return next('/login')
  }

  // 需要管理员权限但角色不是 ADMIN
  if (to.meta.requiresAdmin && role !== 'ADMIN') {
    return next('/403')
  }

  next()
})

export default router
