#include <Arduino.h>
// #include <stdio.h>
// #include <string.h>

// #include <iostream>
// #include <string>

#define LED_BUILTIN 2

void setup() {
    // put your setup code here, to run once:
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    // put your main code here, to run repeatedly:
    digitalWrite(LED_BUILTIN, HIGH);
    delay(2000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(2000);
}
