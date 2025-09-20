#include "ComunicacaoSerial.h"

ComunicacaoSerial::ComunicacaoSerial(int send_pin, int receive_pin, unsigned long baud_rate) 
    : pin_send(send_pin), pin_receive(receive_pin), bit_delay(1000000/baud_rate) {
    flagEnvio = false;
    estado_recebimento = 1;
    info_recebido = 0;
}

void ComunicacaoSerial::iniciar() {
    pinMode(pin_send, OUTPUT);
    pinMode(pin_receive, INPUT);
    digitalWrite(pin_send, HIGH); // Linha ociosa em nível alto
}


void ComunicacaoSerial::enviarByte(byte dado) {
    // bit de start
    digitalWrite(pin_send, LOW);
    delayMicroseconds(bit_delay);

    // bits de dados (8 bits)
    for (int i = 0; i < 8; i++) {
        digitalWrite(pin_send, (dado & 1) ? HIGH : LOW);
        delayMicroseconds(bit_delay);
        dado >>= 1;
    }

    // bit de stop
    digitalWrite(pin_send, HIGH);
    delayMicroseconds(bit_delay);
}

byte ComunicacaoSerial::receberByte() {
    byte dado = 0;
    
    // Espera pelo bit de start (nível baixo)
    while(digitalRead(pin_receive) == HIGH);
    delayMicroseconds(bit_delay/2); // Vai para o meio do bit de start
    
    // Lê 8 bits de dados
    for (int i = 0; i < 8; i++) {
        delayMicroseconds(bit_delay);
        dado >>= 1;
        if (digitalRead(pin_receive)) {
            dado |= 0x80;
        }
    }
    
    // Espera pelo bit de stop
    delayMicroseconds(bit_delay);
    return dado;
}

bool ComunicacaoSerial::transmitirMsg(byte *mensagem, int tam) {
    setFlagEnvio(true);
    if (!flagEnvio) {
        Serial.println("Transmissão ocupada!");
        return false;
    }
    
    Serial.println("Iniciando transmissão...");
    enviarByte(0x02);  // STX
    
    for (int i = 0; i < tam; i++) {
        Serial.print("Enviando caractere ASCII: ");
        Serial.print((char)mensagem[i]);  // Mostra o caractere
        Serial.print(" (");
        Serial.print(mensagem[i]);        // Mostra o valor decimal
        Serial.println(")");
        enviarByte(mensagem[i]);
    }
    
    enviarByte(0x03);  // ETX
    Serial.println("Mensagem enviada com sucesso!");
    
    flagEnvio = false;
    return true;
}

void ComunicacaoSerial::receberDados() {
    static int count_bits = 0;
    
    if (digitalRead(pin_receive) == LOW) {
        byte c = receberByte();
        
        switch(estado_recebimento) {
            case 1:  // Aguardando STX
                if (c == 0x02) {
                    estado_recebimento = 2;
                    bufferRecepcao = "";
                }
                break;
                
            case 2:  // Recebendo dados
                if (c == 0x03) {  // ETX
                    estado_recebimento = 3;
                    processarRecepcao();
                } else {
                    bufferRecepcao += (char)c;
                }
                break;
                
            case 3:  // Processamento
                estado_recebimento = 1;
                break;
        }
    }
}

byte ComunicacaoSerial::recebe(void) {
    if (estado_recebimento == 3) {
        byte temp = info_recebido;
        estado_recebimento = 1;
        return temp;
    }
    return OCUPADO;
}

void ComunicacaoSerial::processarRecepcao() {
    Serial.print("Recebido: ");
    for(unsigned int i = 0; i < bufferRecepcao.length(); i++) {
        Serial.print((char)bufferRecepcao[i]);  // Mostra o caractere
        Serial.print(" (ASCII: ");
        Serial.print((byte)bufferRecepcao[i]);  // Mostra o valor ASCII
        Serial.print(") ");
    }
    Serial.println();
}

void ComunicacaoSerial::enviarBit(bool bit) {
    digitalWrite(pin_send, bit ? HIGH : LOW);
    delayMicroseconds(bit_delay);
}