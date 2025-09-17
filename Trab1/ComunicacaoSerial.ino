
#include "ComunicacaoSerial.h"

ComunicacaoSerial::ComunicacaoSerial(HardwareSerial &tx, HardwareSerial &rx)
  : portaTx(tx), portaRx(rx) {
  bufferRecepcao = "";
}

void ComunicacaoSerial::iniciar(long baudRate) {
  portaTx.begin(baudRate);
  portaRx.begin(baudRate);
}

void ComunicacaoSerial::transmitirMsg(const String &mensagem) {
  portaTx.println(mensagem);
}

void ComunicacaoSerial::receberDados() {
  while (portaRx.available()) {
    char c = portaRx.read();
    bufferRecepcao += c;
    if (c == '\n') {
      processarRecepcao();
      bufferRecepcao = "";
    }
  }
}

void ComunicacaoSerial::processarTransmissao() {
  String msg = "Mensagem via Timer!";
  transmitirMsg(msg); 
}

void ComunicacaoSerial::processarRecepcao() {
  Serial.print("Recebido em outra porta: ");
  Serial.println(bufferRecepcao);
}