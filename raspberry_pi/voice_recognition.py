
import pvporcupine
import pyaudio
import speech_recognition as sr
class VoiceRecognition:
    def __init__(self, keywords=["hey_porcupine"]):
        self.keywords = keywords
        self.porcupine = pvporcupine.create(keywords=self.keywords)
        self.recognizer = sr.Recognizer()
        self.p = pyaudio.PyAudio()
        self.stream = self.p.open(rate=self.porcupine.sample_rate,
                                   channels=1,
                                   format=pyaudio.paInt16,
                                   input=True,
                                   frames_per_buffer=self.porcupine.frame_length)

    def listen_for_wake_word(self):
        print("等待唤醒词...")
        while True:
            pcm = self.stream.read(self.porcupine.frame_length)
            result = self.porcupine.process(pcm)
            if result >= 0:
                print("检测到唤醒词！")
                return True

    def listen_for_command(self):
        with sr.Microphone() as source:
            print("请说话...")
            audio = self.recognizer.listen(source)
        try:
            command = self.recognizer.recognize_google(audio, language='zh-CN')
            print(f"识别到命令: {command}")
            return command
        except sr.UnknownValueError:
            print("无法理解语音")
            return None
        except sr.RequestError:
            print("无法请求Google服务")
            return None
