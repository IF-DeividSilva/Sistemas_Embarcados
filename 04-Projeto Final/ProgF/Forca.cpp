#include <Arduino.h>
#include <string.h>
#include "Tasks.h"

void TaskForca(void *pvParameters) {
  int cmd;
  char letraAtual = 'A';
  int vidas = 0;
  const char* palavraAlvo;
  char palavraTela[6];

  for (;;) {
    vidas = 10;
    letraAtual = 'A';
    int sorteio = random(0, 5);
    palavraAlvo = lista5Letras[sorteio];
    strcpy(palavraTela, "_____");

    bool jogando = true;
    bool venceu = false;

    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    xSemaphoreGive(mutexLCD);

    while(jogando) {
      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.setCursor(0, 0);
      lcd.print(palavraTela);
      lcd.setCursor(10, 0);
      lcd.print("V:"); lcd.print(vidas);
      lcd.setCursor(0, 1);
      lcd.print("Letra: ["); lcd.print(letraAtual); lcd.print("]   ");
      xSemaphoreGive(mutexLCD);

      if (xQueueReceive(filaComandos, &cmd, portMAX_DELAY) == pdPASS) {
        if (cmd == DIREITA) {
          letraAtual++; if (letraAtual > 'Z') letraAtual = 'A';
        } else if (cmd == ESQUERDA) {
          letraAtual--; if (letraAtual < 'A') letraAtual = 'Z';
        } else if (cmd == SELECIONAR || cmd == BAIXO) {
          bool acertou = false;
          bool jaTinha = false;
          for (int i = 0; i < 5; i++) {
             if (palavraAlvo[i] == letraAtual) {
                if (palavraTela[i] == '_') { palavraTela[i] = letraAtual; acertou = true; }
                else jaTinha = true;
             }
          }
          if (!acertou && !jaTinha) {
            vidas--;
            somDerrota();
          }
        }
      }

      bool completa = true;
      for(int i=0; i<5; i++) if(palavraTela[i] == '_') completa = false;
      if (completa) { jogando = false; venceu = true; }
      if (vidas <= 0) { jogando = false; venceu = false; }
    }

    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    if (venceu) {
      lcd.print("YOU WIN! :)");
      lcd.setCursor(0,1);
      lcd.print(palavraAlvo);
      somVitoria();
    } else {
      lcd.print("GAME OVER:(");
      lcd.setCursor(0,1);
      lcd.print("Era: "); lcd.print(palavraAlvo);
      somDerrota();
    }
    xSemaphoreGive(mutexLCD);

    vTaskDelay(3000 / portTICK_PERIOD_MS);
    xQueueReset(filaComandos);
    vTaskResume(handleMenu);
    vTaskSuspend(NULL);
  }
}
