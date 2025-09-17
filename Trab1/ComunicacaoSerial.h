#ifndef COMUNICACAO_SERIAL_H
#define COMUNICACAO_SERIAL_H

#include <Arduino.h>

class ComunicacaoSerial {
  private:
    HardwareSerial &portaTx;     // Porta usada para enviar
    HardwareSerial &portaRx;     // Porta usada para receber
    String bufferRecepcao;       // Buffer para armazenar o que chega

  public:
    ComunicacaoSerial(HardwareSerial &tx, HardwareSerial &rx);
    void iniciar(long baudRate);
    void transmitirMsg(const String &mensagem);
    void receberDados();
    void processarTransmissao();
    void processarRecepcao();
};

#endif
