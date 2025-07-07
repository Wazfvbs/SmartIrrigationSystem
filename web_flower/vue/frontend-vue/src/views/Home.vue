<template>
  <div class="home-container">
    <h2 class="page-title">🌱 我的植物监控中心</h2>

    <el-skeleton v-if="loading" :rows="4" animated />

    <el-carousel
      v-else-if="plants.length > 0"
      :interval="5000"
      type="card"
      height="440px"
      class="plant-carousel"
    >
      <el-carousel-item v-for="plant in plants" :key="plant.plant_id">
        <div class="carousel-card">
          <router-link :to="`/status/${plant.plant_id}`" class="plant-link">
            <PlantCard :plant="plant" />
          </router-link>
        </div>
      </el-carousel-item>
    </el-carousel>

    <el-empty v-else description="暂无绑定植物，请先绑定设备" />
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { getMyPlants, getPlantLatestData } from '@/api/plant'
import PlantCard from '@/components/PlantCard.vue'

const plants = ref([])
const loading = ref(true)

onMounted(async () => {
  try {
    const list = await getMyPlants()
    console.log('✅ 植物列表:', list)

    const enriched = await Promise.all(
      list.map(async (plant) => {
        try {
          const data = await getPlantLatestData(plant.plant_id)
          console.log(`🌿 ${plant.plant_id} 实时数据:`, data)
          return { ...plant, ...data.data }
        } catch (e) {
          console.error(`❌ 加载植物 ${plant.plant_id} 的状态失败`, e)
          return { ...plant, error: true }
        }
      })
    )

    plants.value = enriched
  } catch (err) {
    console.error('❌ 获取植物列表失败:', err)
    ElMessage.error('加载植物失败，请稍后重试')
  } finally {
    loading.value = false
  }
})
</script>

<style scoped>
.home-container {
  padding: 40px 24px;
  background: linear-gradient(to bottom right, #f4f7fb, #ffffff);
  min-height: 100vh;
  color: #333;
  font-family: 'Segoe UI', Tahoma, sans-serif;
  transition: all 0.3s ease;
}

.page-title {
  font-size: 28px;
  font-weight: bold;
  margin-bottom: 30px;
  color: #222;
  text-align: center;
}

/* 美化轮播容器 */
.plant-carousel {
  padding: 20px 0;
}

/* 外壳卡片修饰，保留圆角和阴影 */
.carousel-card {
  overflow: hidden;
  border-radius: 20px;
  height: auto;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 6px 20px rgba(0, 0, 0, 0.08);
  background: transparent;
}

/* 避免 router-link 破坏圆角 */
.plant-link {
  display: block;
  width: 100%;
  height: 100%;
  border-radius: 20px;
  text-decoration: none;
}
</style>
