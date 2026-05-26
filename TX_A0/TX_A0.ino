#define OUR_PIN 3

void setup() {
  pinMode(OUR_PIN, OUTPUT);
}

void loop() {
  // Пример передачи трёх состояний
  int time_delay = 1000;
  sendState(1); delay(time_delay);  // 0V
  sendState(2); delay(time_delay);  // 2.5V
  sendState(3); delay(time_delay);  // 5V
}

void sendState(int state) {
  if (state == 1) {
    analogWrite(OUR_PIN, 0);  // 0V (если поддерживается PWM)
    // или digitalWrite(A0, LOW);
  } else if (state == 2) {
    analogWrite(OUR_PIN, 127);  // ~2.5V (PWM 50% + RC-фильтр)
  } else if (state == 3) {
    analogWrite(OUR_PIN, 255);  // 5V (PWM 100%)
    // или digitalWrite(A0, HIGH);
  }
}
