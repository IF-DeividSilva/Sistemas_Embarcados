#ifndef TASKS_H
#define TASKS_H

#include <Arduino_FreeRTOS.h>
#include "GameDefs.h"

// Prototypes das Tasks
void TaskJoystick(void *pvParameters);
void TaskMenu(void *pvParameters);
void TaskSnake(void *pvParameters);
void TaskDino(void *pvParameters);
void TaskForca(void *pvParameters);
void TaskMusica(void *pvParameters);

// Funções auxiliares de som e desenhar
void somVitoria();
void somDerrota();
void somMenuNavegar();
void somMenuSelecionar();
void desenharPixel(int x, int y, byte vramSnake[32][8]);

#endif
