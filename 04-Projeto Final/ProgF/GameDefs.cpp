#include "GameDefs.h"

// PINAGEM
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

const int pinoJoyX = A0;
const int pinoJoyY = A1;
const int pinoBotao = A2;
const int pinoBuzzer = 48;

// RTOS handles (definições)
QueueHandle_t filaComandos = NULL;
SemaphoreHandle_t mutexLCD = NULL;
TaskHandle_t handleMenu = NULL;
TaskHandle_t handleSnake = NULL;
TaskHandle_t handleDino = NULL;
TaskHandle_t handleForca = NULL;
TaskHandle_t handleMusica = NULL;

// Bitmaps
byte dinoParado[8]  = { B00111, B00111, B00111, B00100, B11111, B00100, B01010, B10001  };
byte dinoPulo[8]    = { B00111, B00111, B00111, B00100, B11111, B10100, B10100, B10000  };
byte cacto[8]       = { B00100, B00101, B10101, B11111, B10100, B10100, B11111, B00100  };
byte ave[8]         = {  B00100, B01110, B11111, B00100, B00000, B00000, B00000, B00000 };

byte blocoCima[8]   = { B11111, B11111, B11111, B11111, B00000, B00000, B00000, B00000 };
byte blocoBaixo[8]  = { B00000, B00000, B00000, B00000, B11111, B11111, B11111, B11111 };
byte blocoCheio[8]  = { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111 };

// Banco de palavras
const int qtdPalavras = 5;
const char* lista5Letras[] = {"NATAL", "FOGAO", "TIGRE", "LIVRO", "VASCO", "FURTO", "MAGIA", "CASCA"};

// Melodia do Dino
int melodiaDino[] = { 
  330, 392, 440, 494, 440, 392, 330, 330, 
  392, 440, 494, 523, 494, 440, 392, 330 
};
int duracoes[] = { 
  150, 150, 150, 150, 150, 150, 300, 150,
  150, 150, 150, 150, 150, 150, 150, 300 
};
int qtdNotas = 16;
