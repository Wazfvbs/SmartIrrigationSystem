import request from '@/utils/request'

export function fetchUsers(page = 1, size = 10, keyword = '') {
  return request.get('/api/admin/users', {
    params: { page, size, keyword }
  })
}

export function blockUser(userId, block) {
  return request.post('/api/admin/users/block', {
    userId,
    block
  })
}
