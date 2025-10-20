#include "ComunicacaoSerial.h"

// Construtor
ComunicacaoSerial::ComunicacaoSerial(HardwareSerial &tx, HardwareSerial &rx)
  : portaTx(tx), portaRx(rx) {
  bufferRecepcao = "";
  mensagemAtual = "UTFPR"; // Define uma mensagem inicial
  novaMensagemRecebida = false;
}

// Inicia a comunicação
void ComunicacaoSerial::iniciar(long baudRate) {
  portaTx.begin(baudRate);
  portaRx.begin(baudRate);
}

// Método Auxiliar: Atualiza a mensagem que será enviada
void ComunicacaoSerial::setMensagemParaEnvio(const String &novaMensagem) {
  mensagemAtual = novaMensagem;
}

// ---- (b) ENVIAR INFORMAÇÃO ----
// Apenas envia a string pela porta serial.
void ComunicacaoSerial::enviarInformacao(const String &mensagem) {
  portaTx.println(mensagem);
}

// ---- (c) PROCESSAR A TRANSMISSÃO ----
// Este método é chamado pela interrupção do Timer1.
// A sua responsabilidade é decidir O QUE e QUANDO enviar.
void ComunicacaoSerial::processarTransmissao() {
  // A lógica de "processamento" aqui é simplesmente enviar a mensagem atual.
  enviarInformacao(mensagemAtual);
}

// ---- (a) RECEBER INFORMAÇÃO ----
// Este método é chamado pela interrupção do Timer3.
// A sua única responsabilidade é ler os bytes e sinalizar que uma mensagem chegou.
// Deve ser o mais rápido possível.
void ComunicacaoSerial::receberInformacao() {
  while (portaRx.available()) {
    char c = portaRx.read();
    bufferRecepcao += c;
    if (c == '\n') {
      novaMensagemRecebida = true;
      break;
    }
  }
}

// ---- (d) PROCESSAR A RECEPÇÃO ----
// Este método é chamado pelo loop() principal.
// Ele verifica a flag e, se uma mensagem chegou, a processa (neste caso, imprime).
void ComunicacaoSerial::processarRecepcao() {
  if (novaMensagemRecebida) {
    // Processamento: Imprimir no monitor serial principal
    Serial.print(F("<<< Mensagem recebida via Serial2: "));
    Serial.print(bufferRecepcao); // bufferRecepcao já tem o '\n'

    // Limpeza para a próxima mensagem
    bufferRecepcao = "";
    novaMensagemRecebida = false;
  }
}