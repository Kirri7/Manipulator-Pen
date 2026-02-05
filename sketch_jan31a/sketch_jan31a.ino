// #include <stdio.h>
// #include <string.h>

// #include <iostream>
// #include <string>

void setup() {
    // put your setup code here, to run once:
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    // put your main code here, to run repeatedly:
    digitalWrite(LED_BUILTIN, HIGH);
    delay(820);
    digitalWrite(LED_BUILTIN, LOW);
    delay(820);
}
