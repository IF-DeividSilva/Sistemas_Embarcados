#include "Sirene.h"
#include "Notas.h"
#include <Arduino.h>

// =============================
//        MELODIAS (Melhoradas)
// =============================

// Música 0 — Padrão (Tipo relógio digital clássico: Bi-Bi-Bi-Bip...)
int melodia_padrao[] = {
  NOTE_A5, 0, NOTE_A5, 0, NOTE_A5, 0, NOTE_F5, NOTE_C6
};
int duracao_padrao[] = {
  100, 50, 100, 50, 100, 50, 200, 400
};

// Música 1 - Marcha Imperial (Star Wars) - Ritmo corrigido
int melodia_imperial[] = {
  NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_DS4, NOTE_AS4, NOTE_G4,
  NOTE_DS4, NOTE_AS4, NOTE_G4
};
int duracao_imperial[] = {
  500, 500, 500,
  350, 150, 500,
  350, 150, 650
};

// Música 2 - Nokia Tune (Grande Valse) - Ritmo original
int melodia_nokia[] = {
  NOTE_E5, NOTE_D5, NOTE_FS4, NOTE_GS4,
  NOTE_CS5, NOTE_B4, NOTE_D4, NOTE_E4,
  NOTE_B4, NOTE_A4, NOTE_CS4, NOTE_E4,
  NOTE_A4
};
int duracao_nokia[] = {
  150, 150, 150, 300,
  150, 150, 150, 300,
  150, 150, 150, 300,
  600
};

// Música 3 - Super Mario Bros (Intro) - Com pausas (0) para o "swing"
int melodia_mario[] = {
  NOTE_E5, NOTE_E5, 0, NOTE_E5, 
  0, NOTE_C5, NOTE_E5, 0,
  NOTE_G5, 0, NOTE_G4
};
int duracao_mario[] = {
  150, 150, 150, 150,
  150, 150, 150, 150,
  300, 300, 300
};

// =============================
//         CLASSE SIRENE
// =============================

Sirene::Sirene(int p) : pin(p) {
  pinMode(pin, OUTPUT);
}

void Sirene::tocar(int songId, QueueHandle_t xQueue) {

  int *melodia;
  int *duracao;
  int tamanho;

  switch (songId) {
    case 0: // Música padrão
      melodia = melodia_padrao;
      duracao = duracao_padrao;
      tamanho = sizeof(melodia_padrao) / sizeof(int);
      break;

    case 1: // Marcha imperial
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
    
    // Toca a nota
    tone(pin, melodia[i], duracao[i]);
    
    // Calcula o tempo total (nota + pausa entre notas)
    // O 1.3 é o fator de pausa que você usava antes
    int tempoEsperaMs = duracao[i] * 1.3;
    
    BuzzerCommand cmd;
    
    // VERIFICAÇÃO INTELIGENTE:
    // "Espia" (Peek) a fila. Se estiver vazia, espera o tempo da nota.
    // Se chegar algo antes do tempo acabar, acorda imediatamente.
    if (xQueuePeek(xQueue, &cmd, pdMS_TO_TICKS(tempoEsperaMs)) == pdTRUE) {
      
      // Se tiver um comando na fila e for STOP
      if (cmd == BUZZER_STOP) {
        noTone(pin); // Corta o som na hora
        return;      // Sai da função imediatamente
      }
    }
    // Se o tempo passou e nada chegou, o loop continua para a próxima nota.
  }
}

void Sirene::parar() {
  noTone(pin);
}