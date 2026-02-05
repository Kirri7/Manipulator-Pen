void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Arduino готов!");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "ON") {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("LED: ВКЛ");
    } 
    else if (command == "OFF") {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("LED: ВЫКЛ");
    }
    else if (command == "PING") {
      Serial.println("PONG");
    }
    else if (command.startsWith("ECHO ")) {
      Serial.println("Эхо: " + command.substring(5));
    }
    else if (command.length() > 0) {
      Serial.println("Неизвестно: " + command);
    }
  }
  delay(10);
}
