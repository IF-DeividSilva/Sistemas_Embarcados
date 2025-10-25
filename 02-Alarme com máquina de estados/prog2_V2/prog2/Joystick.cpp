#include "Joystick.h"
#include <Arduino.h>

Joystick::Joystick(int x, int y) : pinX(x), pinY(y), lastReadX(0), lastReadY(0), debounce(150) {}

int Joystick::lerX() {
  if (millis() - lastReadX > debounce) {
    int valX = analogRead(pinX);
    lastReadX = millis();
    if (valX > 800) return 1;
    if (valX < 200) return -1;
  }
  return 0;
}

int Joystick::lerY() {
  if (millis() - lastReadY > debounce) {
    int valY = analogRead(pinY);
    lastReadY = millis();
    if (valY > 800) return 1;
    if (valY < 200) return -1;
  }
  return 0;
}