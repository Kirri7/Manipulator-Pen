import os
import sys
import json
import pyaudio
from vosk import Model, KaldiRecognizer

# --- НАСТРОЙКИ ---
MODEL_PATH = "./vosk-model-small-ru-0.22"
COMMANDS = {
    "вперёд": "Выполняю движение ВПЕРЁД",
    "назад": "Выполняю движение НАЗАД",
    "выключить": "Система ВЫКЛЮЧЕНА",
    "включить": "Система ВКЛЮЧЕНА"
}

def execute_command(cmd_text):
    """Функция, где происходит логика вашего приложения"""
    action = COMMANDS.get(cmd_text)
    if action:
        print(f"\n>>> [ДЕЙСТВИЕ]: {action}")
    else:
        print(f"\n>>> [ИГНОР]: Сказано '{cmd_text}', но это не команда.")

def main():
    # Проверка наличия модели
    if not os.path.exists(MODEL_PATH):
        print(f"Ошибка: Папка с моделью '{MODEL_PATH}' не найдена!")
        print("Скачайте модель с https://alphacephei.com/vosk/models и распакуйте в текущую папку.")
        sys.exit(1)

    print("Загрузка модели...")
    model = Model(MODEL_PATH)
    
    # Настройки аудио
    samplerate = 16000
    device_info = pyaudio.PyAudio().get_default_input_device_info()
    channels = int(device_info['maxInputChannels'])

    rec = KaldiRecognizer(model, samplerate)

    p = pyaudio.PyAudio()
    stream = p.open(
        format=pyaudio.paInt16,
        channels=1,
        rate=samplerate,
        input=True,
        frames_per_buffer=8000
    )
    stream.start_stream()

    print("\n--- СИСТЕМА ГОТОВА ---")
    print(f"Доступные команды: {', '.join(COMMANDS.keys())}")
    print("Чтобы выйти, нажмите Ctrl+C\n")

    try:
        while True:
            data = stream.read(4000, exception_on_overflow=False)
            
            if rec.AcceptWaveform(data):
                # rec.Result() вызывается, когда обнаружена пауза в речи
                result_json = json.loads(rec.Result())
                text = result_json.get("text", "").strip()

                if text:
                    print(f"Вы сказали: '{text}'")
                    
                    # Проверка на строгое соответствие (чтобы игнорировать "выключить телефон")
                    if text in COMMANDS:
                        execute_command(text)
                    else:
                        # Если в фразе есть команда, но она не одна (например, "включи свет")
                        # Здесь можно добавить логику поиска подстроки, 
                        # но по вашему ТЗ мы игнорируем лишние слова.
                        pass 
            else:
                # rec.PartialResult() выдает текст в процессе говорения (полезно для индикации)
                # partial = json.loads(rec.PartialResult())
                # if partial.get("partial"):
                #     print(f"Слышу: {partial['partial']}", end='\r')
                pass

    except KeyboardInterrupt:
        print("\nЗавершение работы...")
    finally:
        stream.stop_stream()
        stream.close()
        p.terminate()

if __name__ == "__main__":
    main()