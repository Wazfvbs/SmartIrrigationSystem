<template>
    <div class="container">
      <el-card>
        <el-form :inline="true" class="search-bar">
          <el-form-item label="关键词">
            <el-input v-model="keyword" placeholder="搜索用户名/昵称" clearable />
          </el-form-item>
          <el-form-item>
            <el-button type="primary" @click="loadUsers">搜索</el-button>
          </el-form-item>
        </el-form>
  
        <el-table :data="users" style="width: 100%">
          <el-table-column prop="id" label="ID" width="80" />
          <el-table-column prop="username" label="用户名" />
          <el-table-column prop="nickname" label="昵称" />
          <el-table-column prop="role" label="角色" width="100" />
          <el-table-column prop="isBlocked" label="状态" width="100">
            <template #default="scope">
              <el-tag :type="scope.row.isBlocked ? 'danger' : 'success'">
                {{ scope.row.isBlocked ? '已封禁' : '正常' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column label="操作" width="160">
            <template #default="scope">
              <el-button
                size="small"
                type="danger"
                @click="toggleBlock(scope.row)"
              >
                {{ scope.row.isBlocked ? '解封' : '封禁' }}
              </el-button>
            </template>
          </el-table-column>
        </el-table>
  
        <el-pagination
          layout="prev, pager, next"
          :current-page="page"
          :page-size="size"
          :total="total"
          @current-change="handlePageChange"
        />
      </el-card>
    </div>
  </template>
  
  <script setup>
  import { ref, onMounted } from 'vue'
  import { fetchUsers, blockUser } from '@/api/admin'
  import { ElMessage, ElMessageBox } from 'element-plus'
  
  const users = ref([])
  const page = ref(1)
  const size = ref(10)
  const total = ref(0)
  const keyword = ref('')
  
  const loadUsers = async () => {
    try {
      const res = await fetchUsers(page.value, size.value, keyword.value)
      users.value = res.data.content
      total.value = res.data.totalElements
    } catch (e) {
      ElMessage.error('加载用户失败')
    }
  }
  
  const handlePageChange = (newPage) => {
    page.value = newPage
    loadUsers()
  }
  
  const toggleBlock = (user) => {
    const action = user.isBlocked ? '解封' : '封禁'
    ElMessageBox.confirm(`确认要${action}该用户？`, '提示', {
      type: 'warning',
    }).then(async () => {
      try {
        await blockUser(user.id, !user.isBlocked)
        ElMessage.success(`${action}成功`)
        loadUsers()
      } catch (e) {
        ElMessage.error(`${action}失败`)
      }
    })
  }
  
  onMounted(() => loadUsers())
  </script>
  
  <style scoped>
  .container {
    padding: 20px;
  }
  .search-bar {
    margin-bottom: 20px;
  }
  </style>
  