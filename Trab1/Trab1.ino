#include "ComunicacaoSerial.h"

// Configuracoes da biblioteca TimerInterrupt
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0

#define USE_TIMER_1 true
#define USE_TIMER_3 true

#include "TimerInterrupt.h"

// Intervalos para as interrupcoes de timer
#define TIMER1_INTERVAL_MS    2000  // Envio de mensagem a cada 2 segundos
#define TIMER3_INTERVAL_MS    500   // Verificacao de recepcao a cada 0.5 segundos

// --- OBJETOS GLOBAIS ---
ComunicacaoSerial comunicador(Serial1, Serial2);
String mensagemParaEnviar = "Mensagem com metodos separados";

// --- ROTINAS DE INTERRUPCAO  ---

// TimerHandler1: Chama o metodo para PROCESSAR A TRANSMISSAO
#if USE_TIMER_1
void TimerHandler1() {
  comunicador.processarTransmissao();
}
#endif

// TimerHandler3: Chama o metodo para RECEBER INFORMACAO
#if USE_TIMER_3
void TimerHandler3() {
  comunicador.receberInformacao();
}
#endif

// --- FUNCAO DE CONFIGURACAO ---
void setup() {
  Serial.begin(9600);
  while (!Serial);

  comunicador.iniciar(9600);

  // Configura o Timer1 para envio periodico
  ITimer1.init();
  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS, TimerHandler1)) {
    Serial.println(F("OK: Timer1 (processar transmissao) configurado."));
  } else {
    Serial.println(F("ERRO: Falha ao configurar Timer1."));
  }

  // Configura o Timer3 para verificacao de recepcao periodica
  ITimer3.init();
  if (ITimer3.attachInterruptInterval(TIMER3_INTERVAL_MS, TimerHandler3)) {
    Serial.println(F("OK: Timer3 (receber informacao) configurado."));
  } else {
    Serial.println(F("ERRO: Falha ao configurar Timer3."));
  }

  Serial.println(F("---------------------------------------------"));
  Serial.println(F("Para teste: Conecte o pino 18 (TX1) ao pino 17 (RX2)"));
  Serial.println(F("---------------------------------------------"));
}

void loop() {
  // Atualiza a mensagem a ser enviada se o usuario digitar algo novo
  if (Serial.available()) {
    String novaMsg = Serial.readStringUntil('\n');
    novaMsg.trim();
    comunicador.setMensagemParaEnvio(novaMsg); // Usa um metodo setter na classe
    Serial.print(F(">>> Nova mensagem configurada para envio: '"));
    Serial.print(novaMsg);
    Serial.println(F("'"));
  }

  // Chama o metodo para PROCESSAR A INFORMACAO RECEBIDA
  comunicador.processarRecepcao();
}
