#include "Sirene.h"
#include <Arduino.h>

Sirene::Sirene(int p) : pin(p) {
  pinMode(pin, OUTPUT);
}

void Sirene::tocar() {
  tone(pin, 1000);
}

void Sirene::parar() {
  noTone(pin);
}