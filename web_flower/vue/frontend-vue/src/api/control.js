import axios from './http'

export function sendControlCommand(data) {
  return axios.post('/control/send', data)
}
