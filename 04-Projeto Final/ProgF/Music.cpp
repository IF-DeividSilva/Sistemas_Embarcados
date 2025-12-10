#include <Arduino.h>
#include "Tasks.h"

void TaskMusica(void *pvParameters) {
  for (;;) {
    for (int i = 0; i < qtdNotas; i++) {
      tone(pinoBuzzer, melodiaDino[i]);
      vTaskDelay(duracoes[i] / portTICK_PERIOD_MS);
      noTone(pinoBuzzer);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
  }
}
