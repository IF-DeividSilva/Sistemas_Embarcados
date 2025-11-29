#include "Relogio.h"

Relogio::Relogio() : hora(0), minuto(0), segundo(0), songId(0) {}

void Relogio::tick() {
  segundo++;
  if (segundo >= 60) { segundo = 0; minuto++; }
  if (minuto >= 60) { minuto = 0; hora++; }
  if (hora >= 24) hora = 0;
}

void Relogio::ajustar(int h, int m, int s) {
  hora = h;
  minuto = m;
  segundo = s;
}