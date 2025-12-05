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
const int pinoJoyY = A1; // NOVO: Eixo Y necessário para o Snake
const int pinoBotao = A2;


// -----------------------------------------------------------
// Estruturas de Dados
// -----------------------------------------------------------

enum Comando { NENHUM, ESQUERDA, DIREITA, CIMA, BAIXO, SELECIONAR };

struct Ponto {
  int x;
  int y;
};


// ==========================================================
// HANDLES DO FREERTOS (Variáveis de controle)
// ==========================================================
QueueHandle_t filaComandos;
SemaphoreHandle_t mutexLCD; 
TaskHandle_t handleMenu;   
TaskHandle_t handleSnake;   
TaskHandle_t handleDino;
TaskHandle_t handleWordle; 

// ==========================================================
// BITMAPS e Banco de Palavras
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

  // Criação dos recursos do RTOS
  filaComandos = xQueueCreate(10, sizeof(int));
  mutexLCD = xSemaphoreCreateMutex(); // Cria o cadeado

  // Criação das Tasks
  xTaskCreate(TaskJoystick, "Joystick", 128, NULL, 2, NULL); // Prioridade 2 (leitura rápida)
  xTaskCreate(TaskMenu,     "Menu",     200, NULL, 1, &handleMenu);
  xTaskCreate(TaskSnake,    "Snake",    800, NULL, 1, &handleSnake);
  xTaskCreate(TaskDino,     "Dino",     400, NULL, 1, &handleDino); 
  xTaskCreate(TaskForca,   "Forca",   500, NULL, 1, &handleWordle);

  // Starta com todos os games suspenso (pausad), pois o sistema inicia no Menu
  vTaskSuspend(handleSnake); 
  vTaskSuspend(handleDino);
  vTaskSuspend(handleWordle);

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
      
      // Navegação
      if (cmd == DIREITA) {
        opcao++;
        if (opcao > 2) opcao = 2;
      }
      if (cmd == ESQUERDA) {
        opcao--;
        if (opcao < 0) opcao = 0;
      }

      // Renderização do Menu
      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.setCursor(0, 0);
      if (opcao == 0)      lcd.print(">Snake   Dino   ");
      else if (opcao == 1) lcd.print(" Snake  >Dino   ");
      else if (opcao == 2) lcd.print(" Dino   >Wordle ");
      xSemaphoreGive(mutexLCD);

      // Seleção
      if (cmd == SELECIONAR) {
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
        else if (opcao == 2) vTaskResume(handleWordle);
        
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
  Ponto corpo[40]; 
  Ponto comida;
  int tamanho = 3;
  // Direção em PIXELS
  int dirX = 1, dirY = 0; 
  int cmd;

  // Buffer de Video Virtual: 32 blocos (16 colunas * 2 linhas), 8 linhas por bloco
  byte vram[32][8]; 

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
      
      //  Limpa a VRAM (Zera tudo)
      memset(vram, 0, sizeof(vram));

      //  Desenha a Comida
      desenharPixel(comida.x, comida.y, vram);

      //  Desenha a Snake
      for(int i=0; i<tamanho; i++) {
        desenharPixel(corpo[i].x, corpo[i].y, vram);
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
          if (vram[bloco][row] > 0) blocoVazio = false;
        }

        if (!blocoVazio) {
          // Se o bloco tem desenho e ainda temos slots (Max 8)
          if (slotsUsados < 8) {
            lcd.createChar(slotsUsados, vram[bloco]); // Cria o char dinâmico
            
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
    xSemaphoreGive(mutexLCD);
    
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    xQueueReset(filaComandos);
    vTaskResume(handleMenu);
    vTaskSuspend(NULL);
  }
}

// --- Função Auxiliar para plotar pixel na VRAM ---
// Converte coordenada global (80x16) para Bloco+Bit Local
void desenharPixel(int x, int y, byte vram[32][8]) {
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
  vram[indiceBloco][pixelY] |= (1 << (4 - pixelX));
}

// -----------------------------------------------------------
// TASK DINO RUNNER:
// -----------------------------------------------------------
void TaskDino(void *pvParameters) {
  int dinoY = 1;         // 0 = Cima, 1 = Baixo
  int obstaculoX = 15;   // Posição X do inimigo
  int obstaculoY = 1;    // 0 = Ave (Cima), 1 = Cacto (Baixo)
  int score = 0;
  int cmd;
  float velocidade = 400; // Começa mais lento

  for (;;) {
    
    // --- 1. RECARREGAR SPRITES ---
    // Carrega os desenhos necessários para este jogo
    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.createChar(0, dinoParado); // Char 0: Dino
    lcd.createChar(1, ave);        // Char 1: Ave (triangulin)
    lcd.createChar(2, cacto);      // Char 2: Cacto
    lcd.clear(); 
    xSemaphoreGive(mutexLCD);

    // --- 2. Reset do Jogo ---
    dinoY = 1;        // Começa no chão
    obstaculoX = 15;  // Inimigo longe
    obstaculoY = 1;   // Primeiro inimigo é um cacto
    score = 0;
    velocidade = 300;

    bool jogando = true;
    
    while(jogando) {
      
      // --- INPUT (Controle de Faixa) ---
      if (xQueueReceive(filaComandos, &cmd, 0) == pdPASS) {
        if (cmd == CIMA) {
           dinoY = 0; // Vai para o bloco de cima
        }
        else if (cmd == BAIXO) {
           dinoY = 1; // Vai para o bloco de baixo
        }
      }

      // --- MOVIMENTO DO INIMIGO ---
      obstaculoX--;
      
      // Se o inimigo saiu da tela (passou pelo dino)
      if (obstaculoX < 0) {
        obstaculoX = 15; // Volta para o final
        score++;         // Ponto
        
        // Sorteia o próximo inimigo:
        // random(0, 100) > 50 ? Cima : Baixo
        if (random(0, 100) > 50) {
           obstaculoY = 0; // Ave (Cima)
        } else {
           obstaculoY = 1; // Cacto (Baixo)
        }

        // Aumenta dificuldade
        if(velocidade > 80) velocidade -= 10; 
      }

      // --- COLISÃO ---
      // Se estiverem na mesma coluna (1) E na mesma linha (Y)
      if (obstaculoX == 1 && dinoY == obstaculoY) {
        jogando = false;
      }

      // --- DESENHO ---
      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.clear();
      
      // 1. Desenha Dino
      lcd.setCursor(1, dinoY);
      lcd.write((uint8_t)0); 
      
      // 2. Desenha Inimigo
      lcd.setCursor(obstaculoX, obstaculoY);
      if (obstaculoY == 0) {
        lcd.write((uint8_t)1); // Desenha Ave
      } else {
        lcd.write((uint8_t)2); // Desenha Cacto
      }
      
      // 3. Placar
      lcd.setCursor(10, 0);
      lcd.print(score);
      
      xSemaphoreGive(mutexLCD);

      vTaskDelay((int)velocidade / portTICK_PERIOD_MS);
    }

    // --- GAME OVER ---
    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    lcd.print("GAME OVER");
    lcd.setCursor(0,1);
    lcd.print("Pts: "); lcd.print(score);
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
            // Feedback sonoro de erro aqui (tone...)
            // TODO
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
    } else {
      lcd.print("GAME OVER:(");
      lcd.setCursor(0,1);
      lcd.print("Era: "); lcd.print(palavraAlvo);
    }
    xSemaphoreGive(mutexLCD);

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    // Retorna ao Menu
    xQueueReset(filaComandos); // Limpa comandos acumulados
    vTaskResume(handleMenu);
    vTaskSuspend(NULL);
  }
}