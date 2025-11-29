#include "Sirene.h"
#include "Notas.h"
#include <Arduino.h>

// =============================
//        MELODIAS
// =============================

// Música 0 — Padrão (simples)
int melodia_padrao[] = {
  NOTE_C5, NOTE_G4, NOTE_E4, NOTE_A4, NOTE_B4
};
int duracao_padrao[] = {
  200, 200, 200, 200, 300
};
// Música 1 - Marcha imperial
int melodia_imperial[] = {
  NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_DS4, NOTE_AS4, NOTE_G4, NOTE_DS4, NOTE_AS4, NOTE_G4
};

int duracao_imperial[] = {
  300, 300, 300,
  200, 600, 300, 200, 600, 600
};

// Música 2 - Nokia tune
int melodia_nokia[] = {
  NOTE_E5, NOTE_D5, NOTE_FS4, NOTE_GS4,
  NOTE_CS5, NOTE_B4, NOTE_D4, NOTE_E4,
  NOTE_B4, NOTE_A4, NOTE_CS4
};

int duracao_nokia[] = {
  180, 180, 180, 180,
  180, 180, 180, 180,
  180, 180, 180
};

// Música 3 - Aquele Mário
int melodia_mario[] = {
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_C5, NOTE_E5, NOTE_G5,
  NOTE_G4
};

int duracao_mario[] = {
  150, 150, 150,
  150, 150, 150,
  300
};



// =============================
//         CLASSE SIRENE
// =============================

Sirene::Sirene(int p) : pin(p) {
  pinMode(pin, OUTPUT);
}

void Sirene::tocar(int songId) {

  int *melodia;
  int *duracao;
  int tamanho;

  switch (songId) {

    case 0: // Música padrão
      melodia = melodia_padrao;
      duracao = duracao_padrao;
      tamanho = sizeof(melodia_padrao) / sizeof(int);
      break;

    case 1: // Marcha imperia
      melodia = melodia_imperial;
      duracao = duracao_imperial;
      tamanho = sizeof(melodia_imperial) / sizeof(int);
      break;

    case 2: // Nokia tunes
      melodia = melodia_nokia;
      duracao = duracao_nokia;
      tamanho = sizeof(melodia_nokia) / sizeof(int);
      break;

    case 3: // Aquele Mário?
      melodia = melodia_mario;
      duracao = duracao_mario;
      tamanho = sizeof(melodia_mario) / sizeof(int);
      break;  

    default:
      // Se ID inválido → beep curto
      tone(pin, 1000, 200);
      return;
  }

  // Tocar sequências da música
  for (int i = 0; i < tamanho; i++) {
    tone(pin, melodia[i], duracao[i]);
    delay(duracao[i] * 1.3); // pequena pausa entre notas
  }
}

void Sirene::parar() {
  noTone(pin);
}
