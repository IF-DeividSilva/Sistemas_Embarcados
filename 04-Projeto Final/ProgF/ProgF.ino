// Arquivo principal agora apenas inicializa recursos e cria tasks.
#include <Arduino_FreeRTOS.h>
#include "GameDefs.h"
#include "Tasks.h"

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(pinoBotao, INPUT_PULLUP);
  pinMode(pinoBuzzer, OUTPUT);

  // Criação dos recursos do RTOS
  filaComandos = xQueueCreate(10, sizeof(int));
  mutexLCD = xSemaphoreCreateMutex();

  // Criação das Tasks
  xTaskCreate(TaskJoystick, "Joystick", 128, NULL, 2, NULL);
  xTaskCreate(TaskMenu,     "Menu",     200, NULL, 1, &handleMenu);
  xTaskCreate(TaskSnake,    "Snake",    800, NULL, 1, &handleSnake);
  xTaskCreate(TaskDino,     "Dino",     400, NULL, 1, &handleDino);
  xTaskCreate(TaskForca,    "Forca",    500, NULL, 1, &handleForca);
  xTaskCreate(TaskMusica,   "Musica",   200, NULL, 1, &handleMusica);

  // Starta com jogos suspensos
  vTaskSuspend(handleSnake);
  vTaskSuspend(handleDino);
  vTaskSuspend(handleForca);
  vTaskSuspend(handleMusica);

  lcd.createChar(3, blocoCima);
  lcd.createChar(4, blocoBaixo);
  lcd.createChar(5, blocoCheio);

  // Mostra menu inicial
  xSemaphoreTake(mutexLCD, portMAX_DELAY);
  lcd.clear();
  lcd.print(">Snake   Dino");
  xSemaphoreGive(mutexLCD);
}

void loop() { /* vazio - FreeRTOS gerencia tudo */ }