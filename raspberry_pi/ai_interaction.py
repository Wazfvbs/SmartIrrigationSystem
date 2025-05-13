from deepseek_interaction import DeepSeekInteraction

class AIInteraction:
    def __init__(self, api_key):
        # 使用 DeepSeek API
        self.deepseek = DeepSeekInteraction(api_key)

    def get_ai_response(self, command, sensor_data):
        return self.deepseek.get_response(command, sensor_data)
