
import axios from '@/utils/request'

export const getMyPlants = async () => {
  const res = await axios.get('/user/plants')
  return res.data
}

export const getPlantLatestData = async (plant_id) => {
  const res = await axios.get(`/plant/latest?plant_id=${plant_id}`)
  return res.data
}
export function fetchLatestPlantData(plantId) {
  return axios.get('/plant/latest', {
    params: { plant_id: plantId }
  })
}

export function fetchPlantHistory(plantId, start, end) {
  return axios.get('/plant/history', {
    params: { plant_id: plantId, start, end }
  })
}
