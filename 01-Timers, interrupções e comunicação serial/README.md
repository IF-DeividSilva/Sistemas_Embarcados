# Timers, Interrupções e Comunicação Serial

Sistema de comunicação serial assíncrona para Arduino Mega usando timers e interrupções para transmissão e recepção periódica de mensagens.

## Índice

- [Descrição](#descrição)
- [Funcionalidades](#funcionalidades)
- [Hardware Necessário](#hardware-necessário)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Instalação](#instalação)
- [Como Usar](#como-usar)
- [Estrutura de Arquivos](#estrutura-de-arquivos)
- [Detalhes Técnicos](#detalhes-técnicos)
- [Configurações](#configurações)
- [Troubleshooting](#troubleshooting)

##  Descrição

Este projeto implementa um sistema de comunicação serial que utiliza timers e interrupções para gerenciar a transmissão e recepção de dados de forma autônoma e periódica. O sistema é ideal para aplicações que necessitam de comunicação constante entre dispositivos sem bloquear o loop principal do programa.

### Características Principais

- **Transmissão Periódica Automática**: Envia mensagens a cada 2 segundos usando Timer1
- **Recepção Assíncrona**: Verifica dados recebidos a cada 0.5 segundos usando Timer3
- **Arquitetura Orientada a Objetos**: Código organizado e reutilizável
- **Separação de Responsabilidades**: Lógica de comunicação isolada do código principal

## Funcionalidades

### 1. Transmissão Automática
- Envio de mensagens em intervalos regulares configuráveis
- Mensagem padrão inicial: "UTFPR"
- Possibilidade de alterar a mensagem em tempo de execução

### 2. Recepção Assíncrona
- Monitoramento contínuo da porta serial de recepção
- Buffer de recepção para mensagens completas (até `\n`)
- Processamento de mensagens no loop principal

### 3. Configuração Dinâmica
- Alteração de mensagens via Monitor Serial
- Configuração de taxa de transmissão (baud rate)
- Ajuste dos intervalos dos timers

##  Hardware Necessário

### Componentes
- **Arduino Mega 2560** (ou compatível com múltiplas portas seriais)
- Cabo USB para programação e monitor serial
- Jumpers para conexão entre pinos

### Conexões

Para teste em loopback (comunicação interna):
```
Pino 18 (TX1) ─────> Pino 17 (RX2)
```

Para comunicação com outro dispositivo:
```
Arduino 1 (TX1) ───> Dispositivo (RX)
Dispositivo (TX) ──> Arduino 1 (RX2)
GND ───────────────> GND
```

##  Arquitetura do Sistema

### Diagrama de Fluxo

```
┌─────────────────────────────────────────────────┐
│                  LOOP PRINCIPAL                  │
│  • Lê comandos do usuário (Serial)              │
│  • Processa mensagens recebidas                  │
└─────────────────────────────────────────────────┘
                    ▲
                    │ processa
                    │
┌─────────────────┐ │ ┌──────────────────────────┐
│   TIMER 1       │ │ │      TIMER 3              │
│   (2 segundos)  │ │ │   (0.5 segundos)          │
│                 │ │ │                           │
│ Processa        │ │ │ Recebe                    │
│ Transmissão ────┼─┘ │ Informação                │
│      │          │   │      │                    │
│      ▼          │   │      ▼                    │
│  Envia dados    │   │  Lê buffer RX             │
│  via Serial1    │   │  Sinaliza flag            │
└─────────────────┘   └──────────────────────────┘
```

### Separação de Responsabilidades

#### Métodos Executados em Interrupção (ISR)
- `receberInformacao()`: Lê bytes da porta serial, mínimo processamento
- `processarTransmissao()`: Coordena o envio de mensagens

#### Métodos Executados no Loop
- `processarRecepcao()`: Processa mensagens completas recebidas
- Leitura de comandos do usuário

##  Instalação

### Pré-requisitos

1. **Arduino IDE** (versão 1.8.x ou superior) ou **PlatformIO**
2. **Biblioteca TimerInterrupt**
   ```
   Sketch → Include Library → Manage Libraries
   Buscar: "TimerInterrupt"
   Instalar: "TimerInterrupt by Khoi Hoang"
   ```

### Passos de Instalação

1. Clone ou baixe este repositório
2. Abra o arquivo `Trab1.ino` no Arduino IDE
3. Certifique-se de que os arquivos `ComunicacaoSerial.h` e `ComunicacaoSerial.cpp` estão na mesma pasta
4. Selecione a placa: **Tools → Board → Arduino Mega 2560**
5. Selecione a porta COM correta
6. Faça o upload do código

##  Como Usar

### Teste Básico (Loopback)

1. **Conecte os pinos**: Ligue o pino 18 (TX1) ao pino 17 (RX2)
2. **Abra o Monitor Serial** (115200 baud)
3. **Observe as mensagens**: O sistema enviará "UTFPR" a cada 2 segundos e receberá automaticamente

### Alterando a Mensagem

No Monitor Serial, digite uma nova mensagem e pressione Enter:
```
Olá Mundo!
```

Saída esperada:
```
>>> Nova mensagem configurada para envio: 'Olá Mundo!'
<<< Mensagem recebida via Serial2: Olá Mundo!
<<< Mensagem recebida via Serial2: Olá Mundo!
...
```

### Comunicação com Outro Dispositivo

1. Conecte TX1 ao RX do dispositivo remoto
2. Conecte RX2 ao TX do dispositivo remoto
3. Configure o mesmo baud rate (padrão: 9600)
4. O Arduino enviará mensagens automaticamente e exibirá o que receber

## Estrutura de Arquivos

```
01-Timers, interrupções e comunicação serial/
│
├── Trab1.ino                    # Arquivo principal do Arduino
├── ComunicacaoSerial.h          # Header da classe de comunicação
├── ComunicacaoSerial.cpp        # Implementação da classe
│
└── .theia/
    └── launch.json              # Configuração do debugger (opcional)
```

### Descrição dos Arquivos

#### `Trab1.ino`
Arquivo principal que:
- Configura os timers (Timer1 e Timer3)
- Define as rotinas de interrupção
- Gerencia a entrada do usuário
- Orquestra a comunicação

#### `ComunicacaoSerial.h/cpp`
Classe que encapsula toda a lógica de comunicação:
- **Métodos Públicos**:
  - `iniciar(baudRate)`: Inicializa as portas seriais
  - `enviarInformacao(msg)`: Envia uma mensagem
  - `processarTransmissao()`: Chamado pelo Timer1
  - `receberInformacao()`: Chamado pelo Timer3
  - `processarRecepcao()`: Processa mensagens no loop
  - `setMensagemParaEnvio(msg)`: Atualiza a mensagem

##  Detalhes Técnicos

### Timers Utilizados

| Timer  | Intervalo | Função                    | Handler           |
|--------|-----------|---------------------------|-------------------|
| Timer1 | 2000ms    | Processar Transmissão     | TimerHandler1()   |
| Timer3 | 500ms     | Receber Informação        | TimerHandler3()   |

### Portas Seriais

| Porta   | Pinos      | Função         | Baud Rate |
|---------|------------|----------------|-----------|
| Serial  | USB        | Monitor/Debug  | 9600      |
| Serial1 | 18(TX) 19(RX) | Transmissão | 9600      |
| Serial2 | 16(TX) 17(RX) | Recepção    | 9600      |

### Flags e Estados

```cpp
volatile bool novaMensagemRecebida  // Sinaliza recepção completa
String bufferRecepcao                // Acumula bytes recebidos
String mensagemAtual                 // Mensagem a ser transmitida
```

### Fluxo de Dados

#### Transmissão
```
Usuario digita → setMensagemParaEnvio() → mensagemAtual
                                              │
Timer1 dispara → processarTransmissao() → enviarInformacao()
                                              │
                                              ▼
                                          Serial1.println()
```

#### Recepção
```
Dados chegam → Timer3 dispara → receberInformacao()
                                      │
                                      ▼
                              bufferRecepcao += char
                              novaMensagemRecebida = true
                                      │
                                      ▼
                          Loop → processarRecepcao()
                                      │
                                      ▼
                              Serial.print() (exibe)
```

## ⚙️ Configurações

### Alterando Intervalos dos Timers

No arquivo `Trab1.ino`:
```cpp
#define TIMER1_INTERVAL_MS    2000  // Transmissão (ms)
#define TIMER3_INTERVAL_MS    500   // Recepção (ms)
```

### Alterando Baud Rate

No método `setup()`:
```cpp
comunicador.iniciar(9600);  // Altere para 115200, 57600, etc.
```

### Debug dos Timers

Para habilitar mensagens de debug:
```cpp
#define TIMER_INTERRUPT_DEBUG         1
#define _TIMERINTERRUPT_LOGLEVEL_     4
```

##  Troubleshooting

### Problema: Mensagens não são recebidas

**Soluções**:
- Verifique as conexões físicas (TX1 → RX2)
- Confirme que o baud rate está correto em ambas as portas
- Teste com o Monitor Serial em outra porta
- Verifique se há conflito com outras bibliotecas

### Problema: Mensagens corrompidas

**Soluções**:
- Reduza o baud rate (de 115200 para 9600)
- Verifique a qualidade dos jumpers
- Adicione resistores pull-up se necessário
- Aumente o intervalo do Timer3

### Problema: Timers não inicializam

**Soluções**:
- Verifique se a biblioteca TimerInterrupt está instalada corretamente
- Confirme que está usando Arduino Mega (timers específicos)
- Teste apenas um timer por vez
- Verifique mensagens de erro no Monitor Serial

### Problema: Sistema trava ou reinicia

**Soluções**:
- Reduza o processamento dentro das ISRs
- Não use `Serial.print()` dentro de `receberInformacao()`
- Aumente o tamanho do buffer se mensagens forem muito longas
- Verifique se não há conflitos de interrupção

## Notas Importantes

1. **ISRs devem ser rápidas**: Métodos chamados por interrupção devem executar em poucos microssegundos
2. **Variáveis compartilhadas**: Use `volatile` para variáveis acessadas tanto por ISRs quanto pelo loop
3. **Serial dentro de ISR**: Evite usar `Serial.print()` dentro de rotinas de interrupção
4. **Sincronização**: O sistema usa flags para sincronizar ISRs com o loop principal

##  Conceitos Demonstrados

- ✅ Programação de timers em microcontroladores
- ✅ Uso de interrupções (ISR - Interrupt Service Routine)
- ✅ Comunicação serial assíncrona
- ✅ Programação orientada a objetos em Arduino
- ✅ Separação de responsabilidades
- ✅ Sincronização entre ISRs e loop principal
- ✅ Buffer de recepção e processamento de mensagens

##  Referências

- [Arduino Mega 2560 Documentation](https://docs.arduino.cc/hardware/mega-2560)
- [TimerInterrupt Library](https://github.com/khoih-prog/TimerInterrupt)
- [Arduino Serial Reference](https://www.arduino.cc/reference/en/language/functions/communication/serial/)

##  Licença

Projeto desenvolvido para fins educacionais - UTFPR

---

**Desenvolvido para**: Sistemas Embarcados  
**Instituição**: UTFPR - Universidade Tecnológica Federal do Paraná
