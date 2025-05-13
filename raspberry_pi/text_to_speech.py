from gtts import gTTS
import os

class TextToSpeech:
    def __init__(self, lang='zh'):
        self.lang = lang

    def speak_text(self, text):
        tts = gTTS(text=text, lang=self.lang)
        tts.save("response.mp3")
        os.system("mpg321 response.mp3")  # 播放生成的语音文件
