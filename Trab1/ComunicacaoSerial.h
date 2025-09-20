#ifndef COMUNICACAO_SERIAL_H
#define COMUNICACAO_SERIAL_H

#include <Arduino.h>

#define OCUPADO 0xFF
#define LIVRE 0x00

class ComunicacaoSerial {
  private:
    int pin_send;     // Porta usada para enviar
    int pin_receive;     // Porta usada para receber
    unsigned long bit_delay; // Delay entre bits para simular baud rate
    String bufferRecepcao;       // Buffer para armazenar o que chega
    byte info_recebido;
    bool flagEnvio;  // Flag para controle de envio
    int estado_recebimento;
    
    void enviarBit(bool bit);
    void enviarByte(byte dado);
    byte receberBit();
    byte receberByte();

  public:
    ComunicacaoSerial(int send_pin, int receive_pin, unsigned long baud_rate = 9600);
    void iniciar();
    bool transmitirMsg(byte *mensagem, int tam);
    void receberDados();
    void processarRecepcao();
    byte recebe(void);
    void setFlagEnvio(bool flag) { flagEnvio = flag; }  // Método para controlar flag
};

#endif
