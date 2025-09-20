#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0
#define PIN_SEND 3
#define PIN_RECEIVE 6 
#define TIMER1_INTERVAL_MS    1000   // 1 segundo
#define TIMER2_INTERVAL_MS    500    // 0,5 segundo


#define USE_TIMER_1     true

#if ( defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)  || \
      defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_MINI) || defined(ARDUINO_AVR_ETHERNET) || \
      defined(ARDUINO_AVR_FIO) || defined(ARDUINO_AVR_BT)   || defined(ARDUINO_AVR_LILYPAD) || defined(ARDUINO_AVR_PRO)      || \
      defined(ARDUINO_AVR_NG) || defined(ARDUINO_AVR_UNO_WIFI_DEV_ED) || defined(ARDUINO_AVR_DUEMILANOVE) || defined(ARDUINO_AVR_FEATHER328P) || \
      defined(ARDUINO_AVR_METRO) || defined(ARDUINO_AVR_PROTRINKET5) || defined(ARDUINO_AVR_PROTRINKET3) || defined(ARDUINO_AVR_PROTRINKET5FTDI) || \
      defined(ARDUINO_AVR_PROTRINKET3FTDI) )
  #define USE_TIMER_2     true
#else          
  #define USE_TIMER_3     true
#endif

#include "TimerInterrupt.h"
#include "ComunicacaoSerial.h"

String mensagem = "UTFPR";

volatile bool flagEnvio = false;  // Adicionado volatile para variáveis usadas em ISR
volatile bool flagRecepcao = false;

ComunicacaoSerial comunicacao(PIN_SEND, PIN_RECEIVE, 9600);

// Handlers das interrupções
void TimerHandler1() {
    static bool toggle1 = false;
    digitalWrite(LED_BUILTIN, toggle1);
    toggle1 = !toggle1;
    flagEnvio = true;  // Marca que pode enviar
    Serial.println("Timer1 - Enviando...");  // Debug
}

void TimerHandler2() {
    flagRecepcao = true;  // Marca que pode receber
    Serial.println("Timer2 - Recebendo...");  // Debug
}


void setup() {
    pinMode(LED_BUILTIN, OUTPUT);  // Configura LED
    Serial.begin(9600);
    while (!Serial);  // Espera serial estar pronto
    
    comunicacao.iniciar();

    Serial.println(F("Iniciando comunicação com TimerInterrupt..."));

#if USE_TIMER_1
  ITimer1.init();
  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS, TimerHandler1)) {
    Serial.println(F("Timer1 configurado para envio."));
  } else {
    Serial.println(F("Falha ao configurar Timer1."));
  }
#endif

#if USE_TIMER_2
  ITimer2.init();
  if (ITimer2.attachInterruptInterval(TIMER2_INTERVAL_MS, TimerHandler2)) {
    Serial.println(F("Timer2 configurado para recepção."));
  } else {
    Serial.println(F("Falha ao configurar Timer2."));
  }
#elif USE_TIMER_3
  ITimer3.init();
  if (ITimer3.attachInterruptInterval(TIMER2_INTERVAL_MS, TimerHandler2)) {
    Serial.println(F("Timer3 configurado para recepção."));
  } else {
    Serial.println(F("Falha ao configurar Timer3."));
  }
#endif
  Serial.println("Conecte um jumpper nas portas 3 e 6 do Arduino");
  Serial.println("==============================================");
  Serial.println("A mensagem será enviada da porta 3 para a 6");
}

void loop() {
    static byte mensagem[] = {0x53, 0x68, 0x61, 0x72, 0x6B}; // "Shark"
    static int tamanho = sizeof(mensagem)/sizeof(mensagem[0]);

    if (flagEnvio) {
        Serial.println("Tentando enviar mensagem...");  // Debug
        bool resultado = comunicacao.transmitirMsg(mensagem, tamanho);
        if (resultado) {
            Serial.println("Mensagem enviada com sucesso!");  // Debug
        }
        flagEnvio = false;  // Reset flag após tentar enviar
    }

    if (flagRecepcao) {
        Serial.println("Verificando recepção...");  // Debug
        comunicacao.receberDados();
        byte recebido = comunicacao.recebe();
        if (recebido != OCUPADO) {
            Serial.print("Byte recebido: ");
            Serial.println((char)recebido);
        }
        flagRecepcao = false;  // Reset flag após tentar receber
    }

    delay(10);  // Pequeno delay para estabilidade
}
