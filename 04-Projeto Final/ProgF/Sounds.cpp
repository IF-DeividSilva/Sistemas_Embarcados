#include <Arduino.h>
#include "Tasks.h"

void somVitoria() {
  int melodia[] = {262, 330, 392, 523};
  int duracao[] = {150, 150, 150, 600};
  for (int i = 0; i < 4; i++) {
    tone(pinoBuzzer, melodia[i]);
    vTaskDelay(duracao[i] / portTICK_PERIOD_MS);
    noTone(pinoBuzzer);
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void somDerrota() {
  int melodia[] = {392, 330, 262};
  int duracao[] = {200, 200, 500};
  for (int i = 0; i < 3; i++) {
    tone(pinoBuzzer, melodia[i]);
    vTaskDelay(duracao[i] / portTICK_PERIOD_MS);
    noTone(pinoBuzzer);
  }
}

void somMenuNavegar() {
  tone(pinoBuzzer, 1000, 30);
}

void somMenuSelecionar() {
  tone(pinoBuzzer, 1000);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  tone(pinoBuzzer, 1500);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  noTone(pinoBuzzer);
}
