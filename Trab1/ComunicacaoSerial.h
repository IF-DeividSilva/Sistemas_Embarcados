#ifndef COMUNICACAO_SERIAL_H
#define COMUNICACAO_SERIAL_H

#include <Arduino.h>

class ComunicacaoSerial {
private:
  HardwareSerial &portaTx;
  HardwareSerial &portaRx;
  String bufferRecepcao;
  String mensagemAtual; // Armazena a mensagem a ser enviada
  volatile bool novaMensagemRecebida;

public:
  ComunicacaoSerial(HardwareSerial &tx, HardwareSerial &rx);
  void iniciar(long baudRate);

  // (b) Método para enviar informação
  void enviarInformacao(const String &mensagem);

  // (c) Método para processar a transmissão (chamado pelo Timer)
  void processarTransmissao();

  // (a) Método para receber informação (chamado pelo Timer)
  void receberInformacao();

  // (d) Método para processar a recepção (chamado pelo loop)
  void processarRecepcao();
  
  // Método auxiliar para atualizar a mensagem a ser enviada
  void setMensagemParaEnvio(const String &novaMensagem);
};

#endif