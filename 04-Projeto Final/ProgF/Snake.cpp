#include <Arduino.h>
#include "Tasks.h"

Ponto corpo[40];
byte vramSnake[32][8];

void desenharPixel(int x, int y, byte vramSnake[32][8]) {
  if (x < 0 || x >= 80 || y < 0 || y >= 16) return;
  int colunaLCD = x / 5;
  int linhaLCD = y / 8;
  int indiceBloco = (linhaLCD * 16) + colunaLCD;
  int pixelX = x % 5;
  int pixelY = y % 8;
  vramSnake[indiceBloco][pixelY] |= (1 << (4 - pixelX));
}

void TaskSnake(void *pvParameters) {
  Ponto comida;
  int tamanho = 3;
  int dirX = 1, dirY = 0;
  int cmd;

  for (;;) {
    tamanho = 3;
    corpo[0] = {40, 8}; corpo[1] = {39, 8}; corpo[2] = {38, 8};
    dirX = 1; dirY = 0;
    comida.x = random(0, 80);
    comida.y = random(0, 16);
    bool jogando = true;

    while(jogando) {
      if (xQueueReceive(filaComandos, &cmd, 0) == pdPASS) {
        if (cmd == CIMA && dirY == 0)    { dirX = 0; dirY = -1; }
        if (cmd == BAIXO && dirY == 0)   { dirX = 0; dirY = 1; }
        if (cmd == ESQUERDA && dirX == 0){ dirX = -1; dirY = 0; }
        if (cmd == DIREITA && dirX == 0) { dirX = 1; dirY = 0; }
      }

      for (int i = tamanho - 1; i > 0; i--) corpo[i] = corpo[i-1];
      corpo[0].x += dirX;
      corpo[0].y += dirY;

      if (corpo[0].x < 0 || corpo[0].x >= 80 || corpo[0].y < 0 || corpo[0].y >= 16) {
        jogando = false;
      }
      for (int i=1; i<tamanho; i++) {
         if (corpo[0].x == corpo[i].x && corpo[0].y == corpo[i].y) jogando = false;
      }

      if (corpo[0].x == comida.x && corpo[0].y == comida.y) {
        tamanho++;
        if(tamanho > 40) tamanho = 40;
        comida.x = random(0, 80);
        comida.y = random(0, 16);
      }

      memset(vramSnake, 0, sizeof(vramSnake));
      desenharPixel(comida.x, comida.y, vramSnake);
      for(int i=0; i<tamanho; i++) desenharPixel(corpo[i].x, corpo[i].y, vramSnake);

      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.clear();
      int slotsUsados = 0;
      for (int bloco = 0; bloco < 32; bloco++) {
        bool blocoVazio = true;
        for (int row = 0; row < 8; row++) {
          if (vramSnake[bloco][row] > 0) blocoVazio = false;
        }

        if (!blocoVazio) {
          if (slotsUsados < 8) {
            lcd.createChar(slotsUsados, vramSnake[bloco]);
            int lcdCol = bloco % 16;
            int lcdRow = bloco / 16;
            lcd.setCursor(lcdCol, lcdRow);
            lcd.write((uint8_t)slotsUsados);
            slotsUsados++;
          } else {
            int lcdCol = bloco % 16;
            int lcdRow = bloco / 16;
            lcd.setCursor(lcdCol, lcdRow);
            lcd.print("#");
          }
        }
      }
      xSemaphoreGive(mutexLCD);

      vTaskDelay(150 / portTICK_PERIOD_MS);
    }

    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    lcd.print("GAME OVER");
    lcd.setCursor(0,1);
    lcd.print("Pts: "); lcd.print(tamanho-3);
    somDerrota();
    xSemaphoreGive(mutexLCD);

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    xQueueReset(filaComandos);
    vTaskResume(handleMenu);
    vTaskSuspend(NULL);
  }
}
