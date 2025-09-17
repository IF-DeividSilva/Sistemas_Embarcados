#include "ComunicacaoSerial.h"

// Configurações de debug - manter em 0 para produção
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0

// Definir qual timer usar - para Arduino Mega podemos usar Timer1 ou Timer3
#define USE_TIMER_1     true

#if ( defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)  || \
        defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_MINI) ||    defined(ARDUINO_AVR_ETHERNET) || \
        defined(ARDUINO_AVR_FIO) || defined(ARDUINO_AVR_BT)   || defined(ARDUINO_AVR_LILYPAD) || defined(ARDUINO_AVR_PRO)      || \
        defined(ARDUINO_AVR_NG) || defined(ARDUINO_AVR_UNO_WIFI_DEV_ED) || defined(ARDUINO_AVR_DUEMILANOVE) || defined(ARDUINO_AVR_FEATHER328P) || \
        defined(ARDUINO_AVR_METRO) || defined(ARDUINO_AVR_PROTRINKET5) || defined(ARDUINO_AVR_PROTRINKET3) || defined(ARDUINO_AVR_PROTRINKET5FTDI) || \
        defined(ARDUINO_AVR_PROTRINKET3FTDI) )
  #define USE_TIMER_2     false
#else          
  #define USE_TIMER_3     false
#endif

// Incluir a biblioteca - deve ser apenas no arquivo principal
#include "TimerInterrupt.h"

// Configurações do sistema
#define TIMER1_INTERVAL_MS    2000  // Intervalo de 2 segundos
#define TIMER1_FREQUENCY      (float) (1000.0f / TIMER1_INTERVAL_MS)

// Objetos globais
ComunicacaoSerial comunicador(Serial1, Serial2);
String mensagem = "Msg default";

#if USE_TIMER_1

void TimerHandler1()
{
#if (TIMER_INTERRUPT_DEBUG > 1)
  Serial.print("ITimer1 chamado, millis() = "); 
  Serial.println(millis());
#endif

  // Transmitir mensagem via timer
  comunicador.transmitirMsg(mensagem);
}

#endif

void setup()
{ 
  Serial.begin(9600);
  comunicador.iniciar(9600);
  
  while (!Serial);

  Serial.print(F("\nIniciando comunicacao serial no "));
  Serial.println(BOARD_TYPE);
  Serial.println(TIMER_INTERRUPT_VERSION);
  Serial.print(F("CPU Frequency = ")); 
  Serial.print(F_CPU / 1000000); 
  Serial.println(F(" MHz"));

#if USE_TIMER_1

  // Timer0 é usado para micros(), millis(), delay(), etc e não pode ser usado
  // Para MEGA podemos usar Timer 1, 3, 4, 5
  ITimer1.init();

  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS, TimerHandler1))
  {
    Serial.print(F("Timer1 iniciado OK, millis() = ")); 
    Serial.println(millis());
    Serial.print(F("Enviando mensagem a cada "));
    Serial.print(TIMER1_INTERVAL_MS);
    Serial.println(F(" ms"));
  }
  else
  {
    Serial.println(F("Erro ao configurar Timer1. Tente outro timer ou frequência."));
  }

#endif

  Serial.println(F("================================="));
  Serial.println(F("Conecte os pinos para teste:"));
  Serial.println(F("Pino 18 (TX1) -> Pino 17 (RX2)"));
  Serial.println(F("Pino 16 (TX2) -> Pino 19 (RX1)"));
  Serial.println(F("================================="));
  Serial.println(F("Digite uma mensagem e pressione Enter"));
  Serial.println(F("A mensagem será enviada automaticamente a cada 2 segundos"));
  Serial.println();
}

void loop()
{
  // Processar dados recebidos
  comunicador.receberDados();

  // Ler mensagem do usuário via Serial Monitor
  if (Serial.available()) {
    mensagem = Serial.readStringUntil('\n');
    mensagem.trim(); // Remove espaços e quebras de linha extras
    
    Serial.print(F("Nova mensagem configurada: '"));
    Serial.print(mensagem);
    Serial.println(F("'"));
    Serial.println(F("Será enviada automaticamente pelo timer..."));
  }
}