void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
}

void loop() {
  int value = analogRead(A0);
  int state = decodeState(value);
  
  Serial.print("Value: ");
  Serial.print(value);
  Serial.print(" → State: ");
  Serial.println(state);
  
  delay(200);
}

int decodeState(int analogValue) {
  if (analogValue < 100) return 1;       // ~0V
  else if (analogValue < 600) return 2;  // ~2.5V
  else return 3;                         // ~5V
}
