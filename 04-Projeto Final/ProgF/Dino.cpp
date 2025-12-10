#include <Arduino.h>
#include "Tasks.h"

void TaskDino(void *pvParameters) {
  int dinoY = 1; int cactoX = 15; int score = 0; int framesPulo = 0;
  int cmd; bool noAr = false;

  for (;;) {
    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.createChar(0, dinoParado);
    lcd.createChar(1, dinoPulo);
    lcd.createChar(2, cacto);
    lcd.clear();
    xSemaphoreGive(mutexLCD);

    dinoY = 1; cactoX = 15; score = 0; framesPulo = 0; noAr = false;
    float velocidade = 250;

    vTaskResume(handleMusica);

    bool jogando = true;
    while(jogando) {
      if (xQueueReceive(filaComandos, &cmd, 0) == pdPASS) {
        if ((cmd == CIMA || cmd == SELECIONAR) && !noAr) {
           noAr = true; dinoY = 0; framesPulo = 0;
        }
      }
      if (noAr) { framesPulo++; if (framesPulo > 3) { dinoY = 1; noAr = false; } }
      cactoX--; if (cactoX < 0) { cactoX = 15; score++; if(velocidade > 100) velocidade -= 5; }
      if (cactoX == 1 && dinoY == 1) jogando = false;

      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.clear();
      lcd.setCursor(1, dinoY); lcd.write((uint8_t)(noAr ? 1 : 0));
      lcd.setCursor(cactoX, 1); lcd.write((uint8_t)2);
      lcd.setCursor(10, 0); lcd.print(score);
      xSemaphoreGive(mutexLCD);

      vTaskDelay((int)velocidade / portTICK_PERIOD_MS);
    }

    vTaskSuspend(handleMusica);
    noTone(pinoBuzzer);

    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    lcd.print("GAME OVER");
    lcd.setCursor(0,1);
    lcd.print("Score: "); lcd.print(score);
    xSemaphoreGive(mutexLCD);

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    xQueueReset(filaComandos);
    vTaskResume(handleMenu);
    vTaskSuspend(NULL);
  }
}
