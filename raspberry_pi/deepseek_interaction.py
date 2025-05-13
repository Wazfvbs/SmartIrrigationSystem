import requests

class DeepSeekInteraction:
    def __init__(self, api_key, base_url="https://api.deepseek.cn/v1"):
        """
        初始化 DeepSeek API 交互类
        :param api_key: 你的 DeepSeek API 密钥
        :param base_url: DeepSeek API 基础 URL（默认值为 https://api.deepseek.cn/v1）
        """
        self.api_key = api_key
        self.base_url = base_url

    def get_response(self, command, sensor_data):
        """
        向 DeepSeek API 发送请求，获取基于传感器数据和命令的智能回答
        :param command: 用户的语音命令
        :param sensor_data: 传感器数据（温度、湿度等）
        :return: AI 的回答
        """
        url = f"{self.base_url}/chat"
        headers = {
            "Authorization": f"Bearer {self.api_key}",
            "Content-Type": "application/json"
        }
        data = {
            "question": f"当前温度是{sensor_data['T']}°C，湿度是{sensor_data['H']}%，土壤温度是{sensor_data['soil']}°C，水位深度是{sensor_data['water']}cm。{command}",
            "language": "zh",  # 设置中文语境
        }

        try:
            response = requests.post(url, headers=headers, json=data)
            response.raise_for_status()  # 如果响应状态码不是 200，会抛出异常
            ai_response = response.json()
            return ai_response.get("answer", "对不起，我无法理解这个问题。")
        except requests.exceptions.RequestException as e:
            print(f"请求 DeepSeek API 出错: {e}")
            return "系统繁忙，请稍后再试。"
