void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
}

void loop() {
  // int value = analogRead(A0);
  
  long sum = 0;
  const int iters = 10;
  for (int i = 0; i < iters; i++) {
      sum += analogRead(A0);
      delayMicroseconds(50);
    }
    int value = sum / iters;
  
  // Serial.print("Value: ");
  Serial.println(value);
  // Serial.print(" → State: ");
  // Serial.println(decodeState(value));
  
  // delay(100);
}

int decodeState(int analogValue) {
  if (analogValue < 100) return 1;       // ~0V
  else if (analogValue < 850) return 2;  // ~2.5V
  else return 3;                         // ~5V
}
