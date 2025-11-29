#ifndef SIRENE_H
#define SIRENE_H

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h> // Necessário para usar QueueHandle_t

// Movemos o Enum para cá para ser visível tanto no .ino quanto no .cpp
enum BuzzerCommand { BUZZER_START, BUZZER_STOP };

class Sirene {
  private:
    int pin;
  
  public:
    Sirene(int p);
    // Agora a função tocar recebe a Fila para poder "espiar" se tem ordem de parada
    void tocar(int songId, QueueHandle_t xQueue);
    void parar();
};

#endif