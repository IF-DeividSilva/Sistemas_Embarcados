#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h> // Biblioteca para Semáforos e Mutex
#include <LiquidCrystal.h>

// --- Hardware ---
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

const int pinoJoyX = A0;
const int pinoJoyY = A1; // NOVO: Eixo Y necessário para o Snake
const int pinoBotao = A2;

// --- Estruturas de Dados ---
enum Comando { NENHUM, ESQUERDA, DIREITA, CIMA, BAIXO, SELECIONAR };

struct Ponto {
  int x;
  int y;
};

// --- Globais do RTOS ---
QueueHandle_t filaComandos;
SemaphoreHandle_t mutexLCD; // Cadeado para o LCD
TaskHandle_t handleMenu;    // Para poder suspender/resumir
TaskHandle_t handleSnake;   // Para poder suspender/resumir
TaskHandle_t handleDino;
TaskHandle_t handleWordle; 

// --- Desenhos (Bitmaps) ---
byte dinoParado[8]  = { B00111, B00111, B00111, B00100, B11111, B00100, B01010, B10001 };
byte dinoPulo[8]    = { B00111, B00111, B00111, B00100, B11111, B10100, B10100, B10000 };
byte cacto[8]       = { B00100, B00101, B10101, B11111, B10100, B10100, B11111, B00100 };

// --- Banco de Palavras (5 letras) ---
const int qtdPalavras = 5; 
const char* lista5Letras[] = {"NATAL", "FOGAO", "TIGRE", "LIVRO", "VASCO", "FURTO", "MAGIA", "CASCA"};

// --- Protótipos ---
void TaskJoystick(void *pvParameters);
void TaskMenu(void *pvParameters);
void TaskSnake(void *pvParameters);

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(pinoBotao, INPUT_PULLUP);

  // 1. Criação dos recursos do RTOS
  filaComandos = xQueueCreate(10, sizeof(int));
  mutexLCD = xSemaphoreCreateMutex(); // Cria o cadeado

  // 2. Criação das Tasks
  // Note que salvamos os "Handles" (identificadores) das tasks nas variáveis globais
  xTaskCreate(TaskJoystick, "Joystick", 128, NULL, 2, NULL); // Prioridade 2 (leitura rápida)
  xTaskCreate(TaskMenu,     "Menu",     200, NULL, 1, &handleMenu);
  xTaskCreate(TaskSnake,    "Snake",    300, NULL, 1, &handleSnake);
  xTaskCreate(TaskDino,     "Dino",     300, NULL, 1, &handleDino); 
  xTaskCreate(TaskWordle,   "Termo",   350, NULL, 1, &handleWordle);

  // Começamos com a Snake suspensa (pausada), pois o sistema inicia no Menu
  vTaskSuspend(handleSnake); 
  vTaskSuspend(handleDino);
  vTaskSuspend(handleWordle);
}

void loop() {}

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
    else if (y < 200) cmd = CIMA;     // Dependendo da montagem, pode ser BAIXO
    else if (y > 800) cmd = BAIXO;    // Dependendo da montagem, pode ser CIMA
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
  int opcao = 0; // 0=Snake, 1=Dino, 2=Wordle

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
// TASK 3: SNAKE
// -----------------------------------------------------------
void TaskSnake(void *pvParameters) {
  // Variáveis do jogo (ficam na memória da task)
  Ponto corpo[32]; // Cobra máxima de 32 pedaços
  Ponto comida;
  int tamanho = 3;
  int dirX = 1, dirY = 0; // Começa andando para direita
  int cmd;

  for (;;) {
    // --- 1. Inicialização do Jogo (Reset) ---
    tamanho = 3;
    corpo[0] = {3, 0}; corpo[1] = {2, 0}; corpo[2] = {1, 0}; // Posição inicial
    dirX = 1; dirY = 0;
    comida.x = random(0, 16); comida.y = random(0, 2); // Comida aleatória

    // Loop do Jogo rodando
    bool jogando = true;
    while(jogando) {
      
      // --- 2. Input (Não bloqueante) ---
      // xQueueReceive com tempo 0 apenas espia se tem comando
      if (xQueueReceive(filaComandos, &cmd, 0) == pdPASS) {
        if (cmd == CIMA && dirY == 0)    { dirX = 0; dirY = -1; }
        if (cmd == BAIXO && dirY == 0)   { dirX = 0; dirY = 1; }
        if (cmd == ESQUERDA && dirX == 0){ dirX = -1; dirY = 0; }
        if (cmd == DIREITA && dirX == 0) { dirX = 1; dirY = 0; }
      }

      // --- 3. Lógica de Movimento ---
      // Move o corpo (o rabo segue a cabeça)
      for (int i = tamanho - 1; i > 0; i--) {
        corpo[i] = corpo[i-1];
      }
      // Move a cabeça
      corpo[0].x += dirX;
      corpo[0].y += dirY;

      // --- 4. Colisões (Paredes) ---
      // Se bater nas bordas, Game Over
      if (corpo[0].x < 0 || corpo[0].x >= 16 || corpo[0].y < 0 || corpo[0].y >= 2) {
        jogando = false; 
      }

      // --- 5. Comer Comida ---
      if (corpo[0].x == comida.x && corpo[0].y == comida.y) {
        tamanho++;
        if(tamanho > 30) tamanho = 30; // Limite de segurança
        // Gera nova comida
        comida.x = random(0, 16);
        comida.y = random(0, 2);
        // (Dica: aqui você ativaria o Buzzer usando um semáforo)
      }

      // --- 6. Desenhar Frame ---
      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.clear();
      
      // Desenha Comida
      lcd.setCursor(comida.x, comida.y);
      lcd.print("*"); // Comida
      
      // Desenha Cobra
      for(int i=0; i<tamanho; i++) {
        lcd.setCursor(corpo[i].x, corpo[i].y);
        if (i==0) lcd.print("O"); // Cabeça
        else      lcd.print("o"); // Corpo
      }
      xSemaphoreGive(mutexLCD);

      // --- 7. Controle de Velocidade ---
      vTaskDelay(300 / portTICK_PERIOD_MS); // Velocidade do jogo
    }

    // --- Fim de Jogo ---
    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    lcd.print("GAME OVER");
    lcd.setCursor(0,1);
    lcd.print("Pts: "); lcd.print(tamanho-3);
    xSemaphoreGive(mutexLCD);
    
    vTaskDelay(2000 / portTICK_PERIOD_MS); // Mostra pontuação por 2 seg

    // Volta para o Menu
    vTaskResume(handleMenu);
    vTaskSuspend(NULL); // Suspende a si mesmo (Snake)
  }
}

// -----------------------------------------------------------
// TASK 4: DINO RUNNER
// -----------------------------------------------------------
void TaskDino(void *pvParameters) {
  int dinoY = 1;      // 0 = Cima (Pulo), 1 = Baixo (Chão)
  int cactoX = 15;    // Começa na direita
  int score = 0;
  int framesPulo = 0; // Contador para controlar a duração do pulo
  int cmd;
  bool noAr = false;  // Estado do pulo

  for (;;) {
    // --- 1. Reset do Jogo ---
    dinoY = 1;
    cactoX = 15;
    score = 0;
    framesPulo = 0;
    noAr = false;
    float velocidade = 250; // Delay inicial (ms)

    bool jogando = true;
    while(jogando) {
      
      // --- 2. Input (Pulo) ---
      // Aceita CIMA ou SELECIONAR (Botão) para pular
      if (xQueueReceive(filaComandos, &cmd, 0) == pdPASS) {
        if ((cmd == CIMA || cmd == SELECIONAR) && !noAr) {
          noAr = true;
          dinoY = 0;      // Sobe
          framesPulo = 0; // Reseta contador de tempo no ar
          
          // Som de pulo (Opcional)
          // tone(pinoBuzzer, 600, 50);
        }
      }

      // --- 3. Física do Pulo ---
      if (noAr) {
        framesPulo++;
        // O pulo dura 3 frames do jogo. No 4º ele cai.
        if (framesPulo > 3) {
          dinoY = 1; // Cai
          noAr = false;
        }
      }

      // --- 4. Movimento do Cacto ---
      cactoX--;
      if (cactoX < 0) {
        cactoX = 15; // Volta para o começo
        score++;     // Ganha ponto
        
        // Aumenta a dificuldade (velocidade diminui o delay)
        if(velocidade > 100) velocidade -= 5; 
      }

      // --- 5. Colisão ---
      // Se o cacto está na coluna 1 (onde fica o dino) E o dino está no chão (linha 1)
      if (cactoX == 1 && dinoY == 1) {
        jogando = false;
      }

      // --- 6. Desenhar Frame ---
      xSemaphoreTake(mutexLCD, portMAX_DELAY);
      lcd.clear();
      
      // Desenha Chão (pontinhos na linha de baixo)
      // (Opcional, deixa o jogo mais bonito mas consome tempo de CPU)
      // lcd.setCursor(0,1); lcd.print("................");

      // Desenha Dino (Coluna 1 fixa)
      lcd.setCursor(1, dinoY);
      // Se estiver no ar usa char 1, no chão char 0
      lcd.write((uint8_t)(noAr ? 1 : 0)); 
      
      // Desenha Cacto
      lcd.setCursor(cactoX, 1); // Cacto sempre no chão (linha 1)
      lcd.write((uint8_t)2);
      
      // Desenha Score
      lcd.setCursor(10, 0);
      lcd.print(score);
      
      xSemaphoreGive(mutexLCD);

      // --- 7. Velocidade do Jogo ---
      vTaskDelay((int)velocidade / portTICK_PERIOD_MS);
    }

    // --- Fim de Jogo ---
    xSemaphoreTake(mutexLCD, portMAX_DELAY);
    lcd.clear();
    lcd.print("GAME OVER");
    lcd.setCursor(0,1);
    lcd.print("Score: "); lcd.print(score);
    // tone(pinoBuzzer, 200, 500); // Som de morte
    xSemaphoreGive(mutexLCD);
    
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    // Volta para o Menu
    vTaskResume(handleMenu);
    vTaskSuspend(NULL);
  }
}

// -----------------------------------------------------------
// TASK 5: FORCA
// -----------------------------------------------------------
void TaskWordle(void *pvParameters) {
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
      lcd.print("VENCEU! :)");
      lcd.setCursor(0,1);
      lcd.print(palavraAlvo);
    } else {
      lcd.print("PERDEU! :(");
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