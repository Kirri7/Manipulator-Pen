#define LED_BUILTIN 11

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // Пример передачи трёх состояний
  int delay = 2000;
  sendState(1); delay(delay);  // 0V
  sendState(2); delay(delay);  // 2.5V
  sendState(3); delay(delay);  // 5V
}

void sendState(int state) {
  if (state == 1) {
    analogWrite(LED_BUILTIN, 0);  // 0V (если поддерживается PWM)
    // или digitalWrite(A0, LOW);
  } else if (state == 2) {
    analogWrite(LED_BUILTIN, 127);  // ~2.5V (PWM 50% + RC-фильтр)
  } else if (state == 3) {
    analogWrite(LED_BUILTIN, 255);  // 5V (PWM 100%)
    // или digitalWrite(A0, HIGH);
  }
}
