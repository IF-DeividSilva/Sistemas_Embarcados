#include "Botao.h"
#include <Arduino.h>

Botao::Botao(int p) : pin(p), lastPress(0), debounce(250) {
  pinMode(pin, INPUT_PULLUP);
}

bool Botao::pressionado() {
  if (digitalRead(pin) == LOW) {
    if (millis() - lastPress > debounce) {
      lastPress = millis();
      return true;
    }
  }
  return false;
}