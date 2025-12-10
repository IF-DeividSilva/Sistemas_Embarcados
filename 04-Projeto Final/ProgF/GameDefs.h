#ifndef GAMEDEFS_H
#define GAMEDEFS_H

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <LiquidCrystal.h>
#include <queue.h>
#include <semphr.h>

// PINAGEM
extern const int RS, EN, D4, D5, D6, D7;
extern LiquidCrystal lcd;

extern const int pinoJoyX;
extern const int pinoJoyY;
extern const int pinoBotao;
extern const int pinoBuzzer;

// Estruturas
enum Comando { NENHUM, ESQUERDA, DIREITA, CIMA, BAIXO, SELECIONAR };

struct Ponto { int x; int y; };

// RTOS handles
extern QueueHandle_t filaComandos;
extern SemaphoreHandle_t mutexLCD;
extern TaskHandle_t handleMenu;
extern TaskHandle_t handleSnake;
extern TaskHandle_t handleDino;
extern TaskHandle_t handleForca;
extern TaskHandle_t handleMusica;

// Bitmaps
extern byte dinoParado[8];
extern byte dinoPulo[8];
extern byte cacto[8];
extern byte ave[8];

extern byte blocoCima[8];
extern byte blocoBaixo[8];
extern byte blocoCheio[8];

// Banco de palavras
extern const int qtdPalavras;
extern const char* lista5Letras[];

// Musica do dino
extern int melodiaDino[];
extern int duracoes[];
extern int qtdNotas;

#endif
