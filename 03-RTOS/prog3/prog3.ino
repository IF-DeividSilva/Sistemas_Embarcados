// ==========================================================
// BIBLIOTECAS
// ==========================================================
#include <Arduino_FreeRTOS.h>
#include <semphr.h> // Para Mutexes e Semáforos
#include <queue.h>  // Para Filas

#include "Botao.h"
#include "Joystick.h"
#include "Relogio.h"
#include "Sirene.h"
#include "TimerRelogio.h"
#include <LiquidCrystal.h>

// ==========================================================
// CONFIGURAÇÕES E OBJETOS
// ==========================================================
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

const int BUZZER_PIN = 48;

// Objetos (iguais ao seu código)
Botao botao(A2);
Joystick joystick(A0, A1);
Relogio relogio;
Relogio alarme;
Sirene sirene(BUZZER_PIN);

// --- ADICIONE UMA LISTA GLOBAL DE MÚSICAS ---
const char* g_songNames[] = {"Padrao", "Marcha", "Nokia", "Mario"};
const int MAX_SONGS = 4; // Número de músicas na lista acima

// ==========================================================
// VARIÁVEIS DE ESTADO (COMPARTILHADAS)
// ==========================================================
enum Estado {
  IDLE,
  SET_HOURS, SET_MINUTES, SET_SECONDS,
  SET_ALARM_HOURS, SET_ALARM_MINUTES, SET_ALARM_SECONDS,
  PLAY_ALARM, SET_ALARM_SONG
};
Estado estadoAtual = IDLE;

bool alarmeAtivado = true;
bool alarmeDisparado = false;

// Enum para a fila do Buzzer
//enum BuzzerCommand { BUZZER_START, BUZZER_STOP };

// ==========================================================
// HANDLES DO FREERTOS (Variáveis de controle)
// ==========================================================
SemaphoreHandle_t xRelogioMutex;
SemaphoreHandle_t xAlarmeMutex;
SemaphoreHandle_t xEstadoMutex;
SemaphoreHandle_t xClockTickSemaphore;
QueueHandle_t xBuzzerQueue;

// ==========================================================
// TASKS
// ==========================================================
void vTaskInput(void *pvParameters);
void vTaskDisplay(void *pvParameters);
void vTaskClockTick(void *pvParameters);
void vTaskAlarmCheck(void *pvParameters);
void vTaskBuzzerControl(void *pvParameters);
void TimerHandler();

// ==========================================================
// SETUP
// ==========================================================
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  // --- Criação dos Mutexes ---
  // "Cadeados" para proteger as variáveis compartilhadas
  xRelogioMutex = xSemaphoreCreateMutex();
  xAlarmeMutex = xSemaphoreCreateMutex();
  xEstadoMutex = xSemaphoreCreateMutex();

  // --- Criação do Semáforo Binário ---
  // Usado para a ISR do Timer "acordar" a vTaskClockTick
  xClockTickSemaphore = xSemaphoreCreateBinary();

  // --- Criação da Fila ---
  // Fila para enviar comandos (BUZZER_START/STOP) para a vTaskBuzzerControl
  xBuzzerQueue = xQueueCreate(2, sizeof(BuzzerCommand));

  // --- Verificação ---
  if (xRelogioMutex == NULL || xAlarmeMutex == NULL || xEstadoMutex == NULL || xClockTickSemaphore == NULL || xBuzzerQueue == NULL) {
    Serial.println(F("Falha ao criar objetos RTOS..."));
    while(1);
  }

  // --- Criação das Tarefas ---
  xTaskCreate(vTaskInput, "Input", 256, NULL, 2, NULL);         // Prioridade Média
  xTaskCreate(vTaskDisplay, "Display", 256, NULL, 1, NULL);       // Prioridade Baixa
  xTaskCreate(vTaskClockTick, "ClockTick", 128, NULL, 3, NULL);  // Prioridade Alta
  xTaskCreate(vTaskAlarmCheck, "AlarmCheck", 256, NULL, 2, NULL); // Prioridade Média
  xTaskCreate(vTaskBuzzerControl, "Buzzer", 128, NULL, 3, NULL);  // Prioridade Alta

  // --- Inicia o Timer ---
  TimerRelogio::iniciar(TimerHandler);
  // O escalonador (vTaskStartScheduler) é chamado automaticamente
}

// ==========================================================
// LOOP (Vazio)
// ==========================================================
void loop() {

}

// ==========================================================
// ISR DO TIMER (Interrupção)
// ==========================================================
// Esta é a função chamada pelo Timer1 a cada segundo.
void TimerHandler() {
  
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  
  // "Dá" o semáforo para desbloquear a vTaskClockTick
  xSemaphoreGiveFromISR(xClockTickSemaphore, &xHigherPriorityTaskWoken);
  
  // Se uma tarefa de prioridade maior foi acordada, troca de contexto.
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

// ==========================================================
// TAREFA: vTaskClockTick (Processa o tick do relógio)
// ==========================================================
void vTaskClockTick(void *pvParameters) {
  for (;;) {
    // 1. "Dorme" e espera indefinidamente pelo semáforo da ISR
    if (xSemaphoreTake(xClockTickSemaphore, portMAX_DELAY) == pdTRUE) {
      
      // 2. Foi acordado! Trava o mutex do relógio
      if (xSemaphoreTake(xRelogioMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        
        // 3. FAZ O TRABALHO: Chama o tick()
        relogio.tick();
        
        // 4. Libera o mutex
        xSemaphoreGive(xRelogioMutex);
      }
    }
  }
}

// ==========================================================
// TAREFA: vTaskInput (Máquina de Estados)
// ==========================================================
void vTaskInput(void *pvParameters) {
  for (;;) {
    // 1. Lê os inputs (igual ao seu código)
    int joyX = joystick.lerX();
    int joyY = joystick.lerY();
    bool btn = botao.pressionado();

    // 2. Pega o estado atual de forma segura
    Estado localEstado;
    if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
      localEstado = estadoAtual;
      xSemaphoreGive(xEstadoMutex);
    }
    
    BuzzerCommand cmd; // Comando para o buzzer

    // 3. Roda a máquina de estados
    switch (localEstado) {
      case IDLE:
        if (btn) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_HOURS;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        break;

      case SET_HOURS:
        if (joyY != 0) {
          if (xSemaphoreTake(xRelogioMutex, portMAX_DELAY) == pdTRUE) {
            relogio.hora += joyY;
            if (relogio.hora > 23) relogio.hora = 0;
            if (relogio.hora < 0) relogio.hora = 23;
            xSemaphoreGive(xRelogioMutex);
          }
        }
        if (btn) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_ALARM_HOURS;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        if (joyX == 1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_MINUTES;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        break;

      case SET_MINUTES:
        if (joyY != 0) {
          if (xSemaphoreTake(xRelogioMutex, portMAX_DELAY) == pdTRUE) {
            relogio.minuto += joyY;
            if (relogio.minuto > 59) relogio.minuto = 0;
            if (relogio.minuto < 0) relogio.minuto = 59;
            xSemaphoreGive(xRelogioMutex);
          }
        }
        if (joyX == -1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_HOURS;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        if (joyX == 1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_SECONDS;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        break;

      case SET_SECONDS:
        if (joyY != 0) {
          if (xSemaphoreTake(xRelogioMutex, portMAX_DELAY) == pdTRUE) {
            relogio.segundo += joyY;
            if (relogio.segundo > 59) relogio.segundo = 0;
            if (relogio.segundo < 0) relogio.segundo = 59;
            xSemaphoreGive(xRelogioMutex);
          }
        }
        if (joyX == -1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_MINUTES;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        if (joyX == 1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = IDLE;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        break;

      case SET_ALARM_HOURS:
        if (joyY != 0) {
          if (xSemaphoreTake(xAlarmeMutex, portMAX_DELAY) == pdTRUE) {
            alarme.hora += joyY;
            if (alarme.hora > 23) alarme.hora = 0;
            if (alarme.hora < 0) alarme.hora = 23;
            xSemaphoreGive(xAlarmeMutex);
          }
        }  
          if (btn) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_ALARM_SONG;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        if (joyX == 1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_ALARM_MINUTES;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        break;

      case SET_ALARM_MINUTES:
        if (joyY != 0) {
          if (xSemaphoreTake(xAlarmeMutex, portMAX_DELAY) == pdTRUE) {
            alarme.minuto += joyY;
            if (alarme.minuto > 59) alarme.minuto = 0;
            if (alarme.minuto < 0) alarme.minuto = 59;
            xSemaphoreGive(xAlarmeMutex);
          }
        }
        if (joyX == -1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_ALARM_HOURS;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        if (joyX == 1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_ALARM_SECONDS;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        break;

      case SET_ALARM_SECONDS:
        if (joyY != 0) {
          if (xSemaphoreTake(xAlarmeMutex, portMAX_DELAY) == pdTRUE) {
            alarme.segundo += joyY;
            if (alarme.segundo > 59) alarme.segundo = 0;
            if (alarme.segundo < 0) alarme.segundo = 59;
            xSemaphoreGive(xAlarmeMutex);
          }
        }
        if (joyX == -1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = SET_ALARM_MINUTES;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        if (joyX == 1) {
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = IDLE;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        break;

      case PLAY_ALARM:
        // A única responsabilidade desta tarefa no PLAY_ALARM
        // é verificar se o botão de parar foi pressionado.
        if (btn) {
          cmd = BUZZER_STOP;
          // Envia o comando "PARAR" para a fila do buzzer
          xQueueSend(xBuzzerQueue, &cmd, 0); 
          
          if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
            estadoAtual = IDLE;
            xSemaphoreGive(xEstadoMutex);
          }
        }
        break;

        case SET_ALARM_SONG:
          // Navega pelas músicas com o Eixo Y
          if (joyY != 0) {
            if (xSemaphoreTake(xAlarmeMutex, portMAX_DELAY) == pdTRUE) {
              // Adiciona +1 (baixo) ou -1 (cima)
              alarme.songId += joyY; 
            
              // Lógica para "dar a volta" na lista
              if (alarme.songId >= MAX_SONGS) alarme.songId = 0;
              if (alarme.songId < 0) alarme.songId = (MAX_SONGS - 1);
            
              xSemaphoreGive(xAlarmeMutex);
            }
          }
          //  Confirma a seleção com o Eixo X (direita)
          if (joyX == 1) {
            if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
              estadoAtual = IDLE; // Volta para o IDLE
              xSemaphoreGive(xEstadoMutex);
            }
          }
        break;

    } // fim do switch

    // 4. "Dorme" por 50ms para não consumir 100% da CPU
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ==========================================================
// TAREFA: vTaskDisplay (Atualiza o LCD)
// ==========================================================
void vTaskDisplay(void *pvParameters) {
  char buffer[17];
  for (;;) {
    // 1. Pega cópias locais
    Relogio localRelogio;
    Relogio localAlarme;
    Estado localEstado;
    int localSongId; // Variável para guardar o ID da música

    if (xSemaphoreTake(xRelogioMutex, portMAX_DELAY) == pdTRUE) {
      localRelogio = relogio;
      xSemaphoreGive(xRelogioMutex);
    }
    if (xSemaphoreTake(xAlarmeMutex, portMAX_DELAY) == pdTRUE) {
      localAlarme = alarme;
      localSongId = alarme.songId; // Pega o ID da música
      xSemaphoreGive(xAlarmeMutex);
    }
    if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
      localEstado = estadoAtual;
      xSemaphoreGive(xEstadoMutex);
    }

    // 2. Lógica de Display (Hora)
    lcd.setCursor(0, 0);
    sprintf(buffer, "Hora:  %02d:%02d:%02d", localRelogio.hora, localRelogio.minuto, localRelogio.segundo);
    lcd.print(buffer);

    // --- LÓGICA DE DISPLAY (Alarme ou Música) ---
    lcd.setCursor(0, 1);
    if (localEstado == SET_ALARM_SONG) {
      // Se estiver no estado SET_ALARM_SONG, mostra o nome da música
      sprintf(buffer, "Musica: %-10s", g_songNames[localSongId]); // %-10s alinha à esquerda
    } else {
      // Senão, mostra a hora do alarme
      sprintf(buffer, "Alarme:%02d:%02d:%02d", localAlarme.hora, localAlarme.minuto, localAlarme.segundo);
    }
    lcd.print(buffer);

    // 3. Lógica do cursor piscando
    if (localEstado != IDLE && (millis() % 1000 < 500)) {
      switch (localEstado) {
        // ... (cases SET_HOURS, MINUTES, SECONDS...) ...
        case SET_ALARM_HOURS: lcd.setCursor(7, 1); lcd.print("  "); break;
        case SET_ALARM_MINUTES: lcd.setCursor(10, 1); lcd.print("  "); break;
        case SET_ALARM_SECONDS: lcd.setCursor(13, 1); lcd.print("  "); break;

        // --- PISCA NOVO ESTADO ---
        case SET_ALARM_SONG:
          lcd.setCursor(8, 1); // Apaga o nome da música
          lcd.print("        "); 
          break;
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

// ==========================================================
// TAREFA: vTaskAlarmCheck (Verifica o Alarme)
// ==========================================================
void vTaskAlarmCheck(void *pvParameters) {
  for (;;) {
    // 1. Pega cópias locais das variáveis
    Relogio localRelogio;
    Relogio localAlarme;

    if (xSemaphoreTake(xRelogioMutex, portMAX_DELAY) == pdTRUE) {
      localRelogio = relogio;
      xSemaphoreGive(xRelogioMutex);
    }
    if (xSemaphoreTake(xAlarmeMutex, portMAX_DELAY) == pdTRUE) {
      localAlarme = alarme;
      xSemaphoreGive(xAlarmeMutex);
    }

    // 2. A LÓGICA DE VERIFICAÇÃO (do seu 'case IDLE')
    // Se o alarme está ativado, não está disparado, e bateu a hora...
    if (alarmeAtivado && !alarmeDisparado &&
        localRelogio.hora == localAlarme.hora &&
        localRelogio.minuto == localAlarme.minuto &&
        localRelogio.segundo == localAlarme.segundo) {
          
      // Envia o comando "LIGAR" para a fila do buzzer
      BuzzerCommand cmd = BUZZER_START;
      xQueueSend(xBuzzerQueue, &cmd, 0);
      
      // Seta o estado para PLAY_ALARM
      if (xSemaphoreTake(xEstadoMutex, portMAX_DELAY) == pdTRUE) {
        estadoAtual = PLAY_ALARM;
        xSemaphoreGive(xEstadoMutex);
      }
      
      alarmeDisparado = true; // Marca como disparado
    }
    
    // Lógica para "resetar" o alarmeDisparado (igual à sua)
    if (localRelogio.segundo != localAlarme.segundo ||
        localRelogio.minuto != localAlarme.minuto ||
        localRelogio.hora != localAlarme.hora) {
      alarmeDisparado = false;
    }

    // 3. "Dorme" por 500ms.
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// ==========================================================
// TAREFA: vTaskBuzzerControl (Atualizada)
// ==========================================================
void vTaskBuzzerControl(void *pvParameters) {
  BuzzerCommand cmd;
  bool tocando = false; 
  int songToPlay = 0;

  for (;;) {
    TickType_t tempoEspera = tocando ? 0 : portMAX_DELAY;

    if (xQueueReceive(xBuzzerQueue, &cmd, tempoEspera) == pdTRUE) {
      if (cmd == BUZZER_START) {
        tocando = true;
        if (xSemaphoreTake(xAlarmeMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          songToPlay = alarme.songId;
          xSemaphoreGive(xAlarmeMutex);
        }
      } else if (cmd == BUZZER_STOP) {
        tocando = false;
        sirene.parar();
      }
    }

    if (tocando) {
      // AGORA PASSAMOS A FILA PARA DENTRO DO METODO TOCAR
      sirene.tocar(songToPlay, xBuzzerQueue);
      
      // Nota: Como o tocar() agora aborta sozinho quando vê o STOP na fila,
      // ele vai retornar imediatamente. O loop reinicia, o xQueueReceive lá em cima
      // vai pegar o STOP oficialmente da fila, setar tocando = false, e tudo para.
    }
  }
}
