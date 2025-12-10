#include <Arduino.h>
#include "Tasks.h"

void somMenuNavegar();
void somMenuSelecionar();

void TaskMenu(void *pvParameters) {
  int cmd;
  int opcao = 0;

  xSemaphoreTake(mutexLCD, portMAX_DELAY);
  lcd.clear();
  lcd.print(">Snake   Dino");
  xSemaphoreGive(mutexLCD);

  for (;;) {
    if (xQueueReceive(filaComandos, &cmd, portMAX_DELAY) == pdPASS) {
      bool mudou = false;
      if (cmd == DIREITA) {
        opcao++;
        if (opcao > 2) opcao = 2;
        else mudou = true;
        if (opcao == 2) mudou = true;
      }
      if (cmd == ESQUERDA) {
        opcao--;
        if (opcao < 0) opcao = 0;
        else mudou = true;
        if (opcao == 0) mudou = true;
      }

      if (mudou || cmd == DIREITA || cmd == ESQUERDA) {
         somMenuNavegar();
      }

      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.setCursor(0, 0);
      if (opcao == 0)      lcd.print(">Snake   Dino   ");
      else if (opcao == 1) lcd.print(" Snake  >Dino   ");
      else if (opcao == 2) lcd.print(" Dino   >Forca ");
      xSemaphoreGive(mutexLCD);

      if (cmd == SELECIONAR) {
        somMenuSelecionar();

        xSemaphoreTake(mutexLCD, portMAX_DELAY);
        lcd.clear();
        lcd.print("Iniciando...");
        xSemaphoreGive(mutexLCD);
        vTaskDelay(800 / portTICK_PERIOD_MS);

        xQueueReset(filaComandos);

        if (opcao == 0)      vTaskResume(handleSnake);
        else if (opcao == 1) vTaskResume(handleDino);
        else if (opcao == 2) vTaskResume(handleForca);

        vTaskSuspend(NULL);

        xSemaphoreTake(mutexLCD, portMAX_DELAY);
        lcd.clear();
        lcd.print("Escolha o Jogo:");
        xSemaphoreGive(mutexLCD);
      }
    }
  }
}
