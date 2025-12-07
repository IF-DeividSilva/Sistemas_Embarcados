#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h> 
#include <LiquidCrystal.h>


// -----------------------------------------------------------
// PINAGEM
// -----------------------------------------------------------
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

const int pinoJoyX = A0;
const int pinoJoyY = A1; 
const int pinoBotao = A2;
const int pinoBuzzer = 48;

// -----------------------------------------------------------
// Estruturas de Dados
// -----------------------------------------------------------

enum Comando { NENHUM, ESQUERDA, DIREITA, CIMA, BAIXO, SELECIONAR };

struct Ponto {
  int x;
  int y;
};

Ponto corpo[40]; 
byte vramSnake[32][8];

// ==========================================================
// HANDLES DO FREERTOS (Variáveis de controle)
// ==========================================================
QueueHandle_t filaComandos;
SemaphoreHandle_t mutexLCD; 
TaskHandle_t handleMenu;   
TaskHandle_t handleSnake;   
TaskHandle_t handleDino;
TaskHandle_t handleForca; 
TaskHandle_t handleMusica;

// ==========================================================
// BITMAPS , Banco de Palavras e notas das musicas
// ==========================================================

// --- Desenhos (Bitmaps para Dino) ---
byte dinoParado[8]  = { B00111, B00111, B00111, B00100, B11111, B00100, B01010, B10001  };
byte dinoPulo[8]    = { B00111, B00111, B00111, B00100, B11111, B10100, B10100, B10000  };
byte cacto[8]       = { B00100, B00101, B10101, B11111, B10100, B10100, B11111, B00100  };
byte ave[8]         = {  B00100, B01110, B11111, B00100, B00000, B00000, B00000, B00000 };

// --- Desenhos (Bitmaps Para Snake) ---
byte blocoCima[8]   = { B11111, B11111, B11111, B11111, B00000, B00000, B00000, B00000 };
byte blocoBaixo[8]  = { B00000, B00000, B00000, B00000, B11111, B11111, B11111, B11111 };
byte blocoCheio[8]  = { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111 };

// --- Banco de Palavras (5 letras) ---
const int qtdPalavras = 5; 
const char* lista5Letras[] = {"NATAL", "FOGAO", "TIGRE", "LIVRO", "VASCO", "FURTO", "MAGIA", "CASCA"};

// --- Melodia do Dino ---
// Notas em Hz: E4, G4, A4, B4 (Escala pentatônica simples)
int melodiaDino[] = { 
  330, 392, 440, 494, 440, 392, 330, 330, 
  392, 440, 494, 523, 494, 440, 392, 330 
};
int duracoes[] = { 
  150, 150, 150, 150, 150, 150, 300, 150,
  150, 150, 150, 150, 150, 150, 150, 300 
};
int qtdNotas = 16;


// ==========================================================
// TASKS
// ==========================================================
void TaskJoystick(void *pvParameters);
void TaskMenu(void *pvParameters);
void TaskSnake(void *pvParameters);
void TaskDino(void *pvParameters);
void TaskForca(void *pvParameters);



// ==========================================================
// SETUP
// ==========================================================
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(pinoBotao, INPUT_PULLUP);
  pinMode(pinoBuzzer, OUTPUT);

  // Criação dos recursos do RTOS
  filaComandos = xQueueCreate(10, sizeof(int));
  mutexLCD = xSemaphoreCreateMutex(); // Cria o cadeado

  // Criação das Tasks
  xTaskCreate(TaskJoystick, "Joystick", 128, NULL, 2, NULL); // Prioridade 2 (leitura rápida)
  xTaskCreate(TaskMenu,     "Menu",     200, NULL, 1, &handleMenu);
  xTaskCreate(TaskSnake,    "Snake",    800, NULL, 1, &handleSnake);
  xTaskCreate(TaskDino,     "Dino",     400, NULL, 1, &handleDino); 
  xTaskCreate(TaskForca,   "Forca",   500, NULL, 1, &handleForca);
  xTaskCreate(TaskMusica, "Musica", 200, NULL, 1, &handleMusica);

  // Starta com todos os games suspenso (pausad), pois o sistema inicia no Menu
  vTaskSuspend(handleSnake); 
  vTaskSuspend(handleDino);
  vTaskSuspend(handleForca);
  vTaskSuspend(handleMusica);

  lcd.createChar(3, blocoCima);
  lcd.createChar(4, blocoBaixo);
  lcd.createChar(5, blocoCheio);
}


// ==========================================================
// LOOP (vazio)
// ==========================================================
void loop() {

}

// --- TASK 1: Leitura do Joystick (Atualizada para 4 direções) ---
void TaskJoystick(void *pvParameters) {
  int x, y;
  Comando cmd = NENHUM;
  
  for (;;) {
    x = analogRead(pinoJoyX);
    y = analogRead(pinoJoyY);
    cmd = NENHUM;

    // Lógica simples de direção (com "Deadzone" no meio)
    if (x < 200) cmd = ESQUERDA;
    else if (x > 800) cmd = DIREITA;
    else if (y < 200) cmd = CIMA;     
    else if (y > 800) cmd = BAIXO;    
    else if (digitalRead(pinoBotao) == LOW) cmd = SELECIONAR;

    if (cmd != NENHUM) {
      xQueueSend(filaComandos, &cmd, 0); // Envia sem esperar se estiver cheio
      vTaskDelay(150 / portTICK_PERIOD_MS); // Debounce
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// -----------------------------------------------------------
// TASK 2: MENU PRINCIPAL (Gerenciador)
// -----------------------------------------------------------
void TaskMenu(void *pvParameters) {
  int cmd;
  int opcao = 0; // 0=Snake, 1=Dino, 2=Forca

  // Desenho Inicial
  xSemaphoreTake(mutexLCD, portMAX_DELAY);
  lcd.clear();
  lcd.print(">Snake   Dino");
  xSemaphoreGive(mutexLCD);

  for (;;) {
    if (xQueueReceive(filaComandos, &cmd, portMAX_DELAY) == pdPASS) {
      
      bool mudou = false; // Flag para saber se precisa tocar o som

      // Navegação
      if (cmd == DIREITA) {
        opcao++;
        if (opcao > 2) opcao = 2;
        else mudou = true; // Só toca se realmente mudou 
        if (opcao == 2) mudou = true; // Força som no limite
      }
      if (cmd == ESQUERDA) {
        opcao--;
        if (opcao < 0) opcao = 0;
        else mudou = true;
        if (opcao == 0) mudou = true;
      }

      // Toca o som de navegação se moveu o cursor
      if (mudou || cmd == DIREITA || cmd == ESQUERDA) {
         somMenuNavegar();
      }

      // Renderização do Menu
      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.setCursor(0, 0);
      if (opcao == 0)      lcd.print(">Snake   Dino   ");
      else if (opcao == 1) lcd.print(" Snake  >Dino   ");
      else if (opcao == 2) lcd.print(" Dino   >Forca ");
      xSemaphoreGive(mutexLCD);

      // Seleção
      if (cmd == SELECIONAR) {
        
        // Toca som de confirmação
        somMenuSelecionar();

        xSemaphoreTake(mutexLCD, portMAX_DELAY);
        lcd.clear();
        lcd.print("Iniciando...");
        xSemaphoreGive(mutexLCD);
        vTaskDelay(800 / portTICK_PERIOD_MS);
        
        // Limpa fila para o jogo começar limpo
        xQueueReset(filaComandos);

        // Troca de Contexto
        if (opcao == 0)      vTaskResume(handleSnake);
        else if (opcao == 1) vTaskResume(handleDino);
        else if (opcao == 2) vTaskResume(handleForca);
        
        vTaskSuspend(NULL); // Suspende o Menu

        // --- RETORNO AO MENU (Pós Game Over) ---
        // Quando o jogo terminar e der Resume no Menu, o código volta AQUI
        xSemaphoreTake(mutexLCD, portMAX_DELAY);
        lcd.clear();
        lcd.print("Escolha o Jogo:");
        xSemaphoreGive(mutexLCD);
      }
    }
  }
}
// -----------------------------------------------------------
// TASK SNAKE: 
// -----------------------------------------------------------
void TaskSnake(void *pvParameters) {
  Ponto comida;
  int tamanho = 3;
  // Direção em PIXELS
  int dirX = 1, dirY = 0; 
  int cmd;



  for (;;) {
    // --- 1. Reset (Posição em Pixels: X=0..79, Y=0..15) ---
    tamanho = 3;
    // Começa no meio da tela (Pixel 40, 8)
    corpo[0] = {40, 8}; corpo[1] = {39, 8}; corpo[2] = {38, 8}; 
    dirX = 1; dirY = 0;
    
    comida.x = random(0, 80); 
    comida.y = random(0, 16); 

    bool jogando = true;

    while(jogando) {
      // --- 2. Input ---
      if (xQueueReceive(filaComandos, &cmd, 0) == pdPASS) {
        if (cmd == CIMA && dirY == 0)    { dirX = 0; dirY = -1; }
        if (cmd == BAIXO && dirY == 0)   { dirX = 0; dirY = 1; }
        if (cmd == ESQUERDA && dirX == 0){ dirX = -1; dirY = 0; }
        if (cmd == DIREITA && dirX == 0) { dirX = 1; dirY = 0; }
      }

      // --- 3. Movimento ---
      for (int i = tamanho - 1; i > 0; i--) corpo[i] = corpo[i-1];
      corpo[0].x += dirX; 
      corpo[0].y += dirY;

      // --- 4. Colisões (Limites 80x16) ---
      if (corpo[0].x < 0 || corpo[0].x >= 80 || corpo[0].y < 0 || corpo[0].y >= 16) {
        jogando = false; 
      }
      for (int i=1; i<tamanho; i++) {
         if (corpo[0].x == corpo[i].x && corpo[0].y == corpo[i].y) jogando = false;
      }

      // --- 5. Comer ---
      if (corpo[0].x == comida.x && corpo[0].y == comida.y) {
        tamanho++;
        if(tamanho > 40) tamanho = 40;
        comida.x = random(0, 80); 
        comida.y = random(0, 16);
      }

      // --- 6. Renderização no display ---
      
      //  Limpa a vramSnake (Zera tudo)
      memset(vramSnake, 0, sizeof(vramSnake));

      //  Desenha a Comida
      desenharPixel(comida.x, comida.y, vramSnake);

      //  Desenha a Snake
      for(int i=0; i<tamanho; i++) {
        desenharPixel(corpo[i].x, corpo[i].y, vramSnake);
      }

      //  Envia para o LCD (Gerenciamento Dinâmico de Slots)
      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.clear(); // Limpa caracteres antigos
      
      int slotsUsados = 0;
      
      // Varre os 32 blocos da tela (0 a 15 linha 0, 16 a 31 linha 1)
      for (int bloco = 0; bloco < 32; bloco++) {
        bool blocoVazio = true;
        
        // Verifica se tem algum pixel aceso neste bloco 5x8
        for (int row = 0; row < 8; row++) {
          if (vramSnake[bloco][row] > 0) blocoVazio = false;
        }

        if (!blocoVazio) {
          // Se o bloco tem desenho e ainda temos slots (Max 8)
          if (slotsUsados < 8) {
            lcd.createChar(slotsUsados, vramSnake[bloco]); // Cria o char dinâmico
            
            // Calcula posição no LCD
            int lcdCol = bloco % 16;
            int lcdRow = bloco / 16;
            
            lcd.setCursor(lcdCol, lcdRow);
            lcd.write((uint8_t)slotsUsados); // Desenha
            
            slotsUsados++;
          } 
          else {
            // Se acabar os slots (cobra muito espalhada), desenha um quadrado cheio
            // como fallback para não ficar invisível, ou ignora.
            int lcdCol = bloco % 16;
            int lcdRow = bloco / 16;
            lcd.setCursor(lcdCol, lcdRow);
            lcd.print("#"); 
          }
        }
      }
      xSemaphoreGive(mutexLCD);

      vTaskDelay(150 / portTICK_PERIOD_MS); // Snake rápida
    }

    // --- Game Over ---
    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    lcd.print("GAME OVER");
    lcd.setCursor(0,1);
    lcd.print("Pts: "); lcd.print(tamanho-3);
    somDerrota();
    xSemaphoreGive(mutexLCD);
    
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    xQueueReset(filaComandos);
    vTaskResume(handleMenu);
    vTaskSuspend(NULL);
  }
}

// --- Função Auxiliar para plotar pixel na vramSnake ---
// Converte coordenada global (80x16) para Bloco+Bit Local
void desenharPixel(int x, int y, byte vramSnake[32][8]) {
  // Proteção de limites
  if (x < 0 || x >= 80 || y < 0 || y >= 16) return;

  int colunaLCD = x / 5;     // Qual caractere horizontal (0-15)
  int linhaLCD = y / 8;      // Qual linha vertical (0-1)
  int indiceBloco = (linhaLCD * 16) + colunaLCD; // 0 a 31

  int pixelX = x % 5;        // Pixel dentro do bloco (0-4)
  int pixelY = y % 8;        // Linha dentro do bloco (0-7)

  // O LCD desenha bits da esquerda para direita:
  // Bit 4 é o mais à esquerda, Bit 0 o mais à direita
  // (1 << (4 - pixelX)) acende o bit correto
  vramSnake[indiceBloco][pixelY] |= (1 << (4 - pixelX));
}

// -----------------------------------------------------------
// TASK DINO RUNNER:
// -----------------------------------------------------------
void TaskDino(void *pvParameters) {
  // ... (variáveis iguais as de antes: dinoY, cactoX, etc) ...
  int dinoY = 1; int cactoX = 15; int score = 0; int framesPulo = 0; 
  int cmd; bool noAr = false;

  for (;;) {
    
    // 1. Recarregar Sprites (Igual fizemos antes)
    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.createChar(0, dinoParado);
    lcd.createChar(1, dinoPulo);
    lcd.createChar(2, cacto);
    lcd.clear(); 
    xSemaphoreGive(mutexLCD);

    // --- 2. START DO JOGO ---
    dinoY = 1; cactoX = 15; score = 0; framesPulo = 0; noAr = false;
    float velocidade = 250; 

    // >>> LIGA A MÚSICA AQUI <<<
    vTaskResume(handleMusica); 

    bool jogando = true;
    while(jogando) {
      // ... (TODA A LÓGICA DE INPUT, PULO, COLISÃO IGUALZINHO ANTES) ...
      // ... (Copie o miolo do while que você já tem funcionando) ...
      
      // Input...
      if (xQueueReceive(filaComandos, &cmd, 0) == pdPASS) {
        if ((cmd == CIMA || cmd == SELECIONAR) && !noAr) {
           noAr = true; dinoY = 0; framesPulo = 0;
           // OBS: Se você tinha tone() de pulo aqui, REMOVA. 
           // O Arduino não toca duas coisas ao mesmo tempo.
        }
      }
      
      // Física... Movimento... Colisão...
      if (noAr) { framesPulo++; if (framesPulo > 3) { dinoY = 1; noAr = false; } }
      cactoX--; if (cactoX < 0) { cactoX = 15; score++; if(velocidade > 100) velocidade -= 5; }
      if (cactoX == 1 && dinoY == 1) jogando = false;

      // Renderização...
      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.clear();
      lcd.setCursor(1, dinoY); lcd.write((uint8_t)(noAr ? 1 : 0)); 
      lcd.setCursor(cactoX, 1); lcd.write((uint8_t)2);
      lcd.setCursor(10, 0); lcd.print(score);
      xSemaphoreGive(mutexLCD);

      vTaskDelay((int)velocidade / portTICK_PERIOD_MS);
    }

    // --- GAME OVER ---
    
    // >>> DESLIGA A MÚSICA AQUI <<<
    vTaskSuspend(handleMusica); 
    noTone(pinoBuzzer); // Garante que o buzzer para de gritar imediatamente
    
    // Som de derrota (Opcional, toca depois que a musica para)
    // tone(pinoBuzzer, 200, 500); 

    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    lcd.print("GAME OVER");
    lcd.setCursor(0,1);
    lcd.print("Score: "); lcd.print(score);
    xSemaphoreGive(mutexLCD);
    
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    xQueueReset(filaComandos);
    vTaskResume(handleMenu);
    vTaskSuspend(NULL);
  }
}
// -----------------------------------------------------------
// TASK 5: FORCA
// -----------------------------------------------------------
void TaskForca(void *pvParameters) {
  int cmd;
  char letraAtual = 'A';
  int vidas = 0;
  int indicePalavra = 0;
  
  // Buffer para a palavra sendo adivinhada (ex: "_ _ A _ _")
  char palavraTela[6]; 
  const char* palavraAlvo;

  for (;;) {
    // --- 1. Setup da Partida ---
    vidas = 10;
    letraAtual = 'A';
    
    // Escolhe uma palavra aleatória da lista
    int sorteio = random(0, 5); // Temos 5 palavras na lista5Letras
    palavraAlvo = lista5Letras[sorteio];
    
    // Preenche a tela com underlines
    strcpy(palavraTela, "_____");

    bool jogando = true;
    bool venceu = false;
    
    // Desenho inicial
    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    xSemaphoreGive(mutexLCD);

    while(jogando) {
      
      // --- 2. Interface Gráfica ---
      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      // Linha 0: Palavra e Vidas
      lcd.setCursor(0, 0);
      lcd.print(palavraTela);
      lcd.setCursor(10, 0);
      lcd.print("V:"); lcd.print(vidas);
      
      // Linha 1: Seletor de Letras
      lcd.setCursor(0, 1);
      lcd.print("Letra: ["); 
      lcd.print(letraAtual); 
      lcd.print("]   ");
      xSemaphoreGive(mutexLCD);

      // --- 3. Input ---
      if (xQueueReceive(filaComandos, &cmd, portMAX_DELAY) == pdPASS) {
        
        // Trocar Letra
        if (cmd == DIREITA) {
          letraAtual++;
          if (letraAtual > 'Z') letraAtual = 'A';
        }
        else if (cmd == ESQUERDA) {
          letraAtual--;
          if (letraAtual < 'A') letraAtual = 'Z';
        }
        
        // Chutar Letra
        else if (cmd == SELECIONAR || cmd == BAIXO) {
          bool acertou = false;
          bool jaTinha = false;
          
          // Verifica se a letra existe na palavra alvo
          for (int i = 0; i < 5; i++) {
             // Se a letra bater e aquele espaço ainda for um underline (não adivinhado)
             if (palavraAlvo[i] == letraAtual) {
                if (palavraTela[i] == '_') {
                   palavraTela[i] = letraAtual; // Revela a letra
                   acertou = true;
                } else {
                   jaTinha = true; // Jogador chutou letra repetida certa
                }
             }
          }

          if (!acertou && !jaTinha) {
            vidas--; // Errou
            somDerrota();
          }
        }
      }

      // --- 4. Verificação de Vitória/Derrota ---
      
      // Checa se ainda tem underlines
      bool completa = true;
      for(int i=0; i<5; i++) {
        if(palavraTela[i] == '_') completa = false;
      }
      
      if (completa) {
        jogando = false;
        venceu = true;
      }
      
      if (vidas <= 0) {
        jogando = false;
        venceu = false;
      }
    }

    // --- Fim de Jogo ---
    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    if (venceu) {
      lcd.print("YOU WIN! :)");
      lcd.setCursor(0,1);
      lcd.print(palavraAlvo);
      somVitoria();
    } else {
      lcd.print("GAME OVER:(");
      lcd.setCursor(0,1);
      lcd.print("Era: "); lcd.print(palavraAlvo);
      somDerrota();
    }
    xSemaphoreGive(mutexLCD);

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    // Retorna ao Menu
    xQueueReset(filaComandos); // Limpa comandos acumulados
    vTaskResume(handleMenu);
    vTaskSuspend(NULL);
  }
}
// --- Função Auxiliar de Som ---
void somVitoria() {
  // Notas: Do, Mi, Sol, Do(Agudo)
  int melodia[] = {262, 330, 392, 523};
  int duracao[] = {150, 150, 150, 600};

  for (int i = 0; i < 4; i++) {
    tone(pinoBuzzer, melodia[i]);
    // Usa vTaskDelay para não travar o RTOS enquanto toca
    vTaskDelay(duracao[i] / portTICK_PERIOD_MS);
    noTone(pinoBuzzer); // Para o som
    vTaskDelay(50 / portTICK_PERIOD_MS); // Pequena pausa entre notas
  }
}

void somDerrota() {
  // Notas: Sol, Mi, Do (Descendo)
  int melodia[] = {392, 330, 262}; 
  int duracao[] = {200, 200, 500};

  for (int i = 0; i < 3; i++) {
    tone(pinoBuzzer, melodia[i]);
    vTaskDelay(duracao[i] / portTICK_PERIOD_MS);
    noTone(pinoBuzzer);
  }
}


// -----------------------------------------------------------
// TASK MUSICA 
// -----------------------------------------------------------
void TaskMusica(void *pvParameters) {
  for (;;) {
    for (int i = 0; i < qtdNotas; i++) {
      // Toca a nota
      tone(pinoBuzzer, melodiaDino[i]);

      // Calcula o tempo que a nota fica soando
      // O Delay libera o processador para o Jogo do Dino rodar
      vTaskDelay(duracoes[i] / portTICK_PERIOD_MS);
      
      // Pequena pausa entre notas para dar efeito de "staccato"
      noTone(pinoBuzzer);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    // Quando acaba o loop for, ele volta pro começo automaticamente (loop infinito)
  }
}

// --- Sons do Menu ---
void somMenuNavegar() {
  // Um "Bip" curto e agudo (1000Hz por 30ms)
  tone(pinoBuzzer, 1000, 30);
  // Não precisa de delay aqui, pois o som é muito curto
}

void somMenuSelecionar() {
  // Som estilo "Moeda" ou "Start" (subindo o tom)
  tone(pinoBuzzer, 1000);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  tone(pinoBuzzer, 1500);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  noTone(pinoBuzzer);
}