#include <Arduino.h>
#include "Tasks.h"

// Joystick Task
void TaskJoystick(void *pvParameters) {
  int x, y;
  Comando cmd = NENHUM;
  for (;;) {
    x = analogRead(pinoJoyX);
    y = analogRead(pinoJoyY);
    cmd = NENHUM;

    if (x < 200) cmd = ESQUERDA;
    else if (x > 800) cmd = DIREITA;
    else if (y < 200) cmd = CIMA;
    else if (y > 800) cmd = BAIXO;
    else if (digitalRead(pinoBotao) == LOW) cmd = SELECIONAR;

    if (cmd != NENHUM) {
      xQueueSend(filaComandos, &cmd, 0);
      vTaskDelay(150 / portTICK_PERIOD_MS);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
