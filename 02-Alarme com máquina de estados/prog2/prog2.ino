// ==========================================================
// BIBLIOTECAS
// ==========================================================
#include <LiquidCrystal.h> // Para o Display LCD

// ==========================================================
// CONFIGURAÇÕES E PINOS
// ==========================================================
// Pinos do LCD (ajuste conforme sua ligação)
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Pinos do Joystick e Buzzer
const int JOY_BUTTON_PIN = A2;
const int JOY_X_PIN = A0; // Eixo X (Esquerda/Direita) 
const int JOY_Y_PIN = A1; // Eixo Y (Cima/Baixo)
const int BUZZER_PIN = 48;

// Constantes de controle
const long JOYSTICK_DEBOUNCE_INTERVAL = 150; // Delay em ms para ler o joystick de novo
const long BUTTON_DEBOUNCE_INTERVAL = 250;   // Delay em ms para ler o botão de novo

// ==========================================================
// VARIÁVEIS GLOBAIS
// ==========================================================
// Variáveis de tempo
volatile int hour = 0; // 'volatile' é importante pois são alteradas na interrupção do timer
volatile int min = 0;
volatile int sec = 0;

// Variáveis do alarme
int alarmHour = 0;
int alarmMin = 0;
int alarmSec = 0;
bool alarmeAtivado = true; // Para poder ativar/desativar o alarme no futuro
int aux = 0;
static bool alarmeDisparado = false; // flag estática 

// Variáveis de debounce
unsigned long lastJoystickRead = 0;
unsigned long lastJoystickRead1 = 0;
unsigned long lastButtonPress = 0;

// Estados da Máquina
enum States {
  IDLE,
  SET_HOURS,
  SET_MINUTES,
  SET_SECONDS,
  SET_ALARM_HOURS,
  SET_ALARM_MINUTES,
  SET_ALARM_SECONDS,
  PLAY_ALARM
};
States estadoAtual = IDLE;

#define USE_TIMER_1 true
#include "TimerInterrupt.h" // Para o nosso relógio preciso
// ==========================================================
// SETUP
// ==========================================================
void setup() {
  Serial.begin(9600);
  
  // Inicialização do display LCD
  lcd.begin(16, 2);
  
  // Inicialização do joystick e botão
  pinMode(JOY_BUTTON_PIN, INPUT_PULLUP);
  
  // Inicialização do buzzer
  pinMode(BUZZER_PIN, OUTPUT);
 

  // Inicializa o Timer1 para disparar a cada 1 segundo (1000 ms)
  ITimer1.init();
  if (ITimer1.attachInterruptInterval(1000, TimerHandler)) {
    Serial.println("Timer do relógio iniciado com sucesso!");
  } else {
    Serial.println("Erro ao iniciar o timer do relógio.");
  }
}

// ==========================================================
// LOOP
// ==========================================================
void loop() {
  maquinaDeEstados(); // A lógica principal está aqui
  updateLCD();        // Atualiza o display constantemente
}

// ==========================================================
// FUNÇÃO DO TIMER (INTERRUPÇÃO)
// Esta função é chamada automaticamente a cada segundo
// ==========================================================
void TimerHandler() {
  sec++;
  if (sec >= 60) {
    sec = 0;
    min++;
    if (min >= 60) {
      min = 0;
      hour++;
      if (hour >= 24) {
        hour = 0;
      }
    }
  }
}

// ==========================================================
// MÁQUINA DE ESTADOS PRINCIPAL
// ==========================================================
void maquinaDeEstados() {
  int joyY = lerJoystickY(); // Lê a posição do joystick (CIMA, BAIXO ou CENTRO)
  int joyX = lerJoystickX(); // le a posiçao (esquerda - direita) horizontal
  switch (estadoAtual) {
    case IDLE:
      // TRANSIÇÃO: Verifica se o botão foi pressionado para entrar no modo de ajuste
      if (botaoPressionado()) {
        estadoAtual = SET_HOURS;
       // aux = 1;
      }
      // TRANSIÇÃO: Verifica se é hora do alarme
      if (alarmeAtivado && hour == alarmHour && min == alarmMin && sec == alarmSec && !alarmeDisparado) {
        estadoAtual = PLAY_ALARM;
        alarmeDisparado = true; // marca que o alarme já tocou
      }
      // Quando o relógio “passa” do horário do alarme, libera novamente
      if (sec != alarmSec || min != alarmMin || hour != alarmHour) {
        alarmeDisparado = false;
      }
      break;

    case SET_HOURS:
      // AÇÃO: Ajusta a hora com o joystick
      if (joyY != 0) { // Se o joystick foi movido
        hour = hour + joyY; // Soma +1 ou -1
        if (hour > 23) hour = 0;
        if (hour < 0) hour = 23;
      }
      // TRANSIÇÃO: Se o botão for pressionado, avança para minutos
      if (botaoPressionado()) {
        estadoAtual = SET_MINUTES;
      }
      break;
      
    case SET_MINUTES:
      // AÇÃO: Ajusta os minutos com o joystick
      if (joyY != 0) {
        min = min + joyY;
        if (min > 59) min = 0;
        if (min < 0) min = 59;
      }
      // TRANSIÇÃO: Se o botão for pressionado, avança para horas
      if (joyX == -1){
        estadoAtual = SET_HOURS;
      }
      // TRANSIÇÃO: Se o botão for pressionado, avança para segundos
      if (botaoPressionado()) {
        estadoAtual = SET_SECONDS;
      }
      break;

    case SET_SECONDS:
      // AÇÃO: Ajusta os segundos com o joystick
      if (joyY != 0) {
        sec = sec + joyY;
        if (sec > 59) sec = 0;
        if (sec < 0) sec = 59;
      }
      // TRANSIÇÃO: Se o botão for pressionado, avança para minutos
      if (joyX == -1){
        estadoAtual = SET_MINUTES;
      }
      // TRANSIÇÃO: Se o botão for pressionado, avança para o ajuste do alarme
      if (botaoPressionado()) {
        estadoAtual = SET_ALARM_HOURS;
      }
      break;
      
    case SET_ALARM_HOURS:
      // AÇÃO: Ajusta a hora do alarme
      if (joyY != 0) {
        alarmHour = alarmHour + joyY;
        if (alarmHour > 23) alarmHour = 0;
        if (alarmHour < 0) alarmHour = 23;
      }
      // TRANSIÇÃO: Avança para minutos do alarme
      if (botaoPressionado()) {
        estadoAtual = SET_ALARM_MINUTES;
      }
      break;
      
    case SET_ALARM_MINUTES:
      // AÇÃO: Ajusta os minutos do alarme
      if (joyY != 0) {
        alarmMin = alarmMin + joyY;
        if (alarmMin > 59) alarmMin = 0;
        if (alarmMin < 0) alarmMin = 59;
      }
      // AÇÃO: Volta para ajuste horas do alarme
      if (joyX == -1){
        estadoAtual = SET_ALARM_HOURS;
      }
      // TRANSIÇÃO: Avança para segundos do alarme
      if (botaoPressionado()) {
        estadoAtual = SET_ALARM_SECONDS;
      }
      break;

    case SET_ALARM_SECONDS:
      // AÇÃO: Ajusta os segundos do alarme
      if (joyY != 0) {
        alarmSec = alarmSec + joyY;
        if (alarmSec > 59) alarmSec = 0;
        if (alarmSec < 0) alarmSec = 59;
      }
      // AÇÃO: Volta para ajuste minutos do alarme
      if (joyX == -1){
        estadoAtual = SET_ALARM_MINUTES;
      }
      // TRANSIÇÃO: Finaliza o ajuste e volta para IDLE
      if (botaoPressionado()) {
        estadoAtual = IDLE;
      }
      break;

    case PLAY_ALARM:
      // AÇÃO: Toca o buzzer
      Serial.println("passou pelo play alarm!");
      tone(BUZZER_PIN, 1000); // Toca uma nota de 1000 Hz
      
      // TRANSIÇÃO: Se o botão for pressionado, desliga o alarme e volta para IDLE
      if (botaoPressionado()) {
        noTone(BUZZER_PIN); // Desliga o buzzer
        estadoAtual = IDLE;
      }
      break;
  }
}


// ==========================================================
// FUNÇÕES AUXILIARES
// ==========================================================

// Função que atualiza tudo que aparece no LCD
void updateLCD() {
  char buffer[17]; // Buffer para formatar o texto
  
  // Linha 1: Hora Atual
  lcd.setCursor(0, 0);
  sprintf(buffer, "Hora:  %02d:%02d:%02d", hour, min, sec);
  lcd.print(buffer);

  // Linha 2: Hora do Alarme
  lcd.setCursor(0, 1);
  sprintf(buffer, "Alarme:%02d:%02d:%02d", alarmHour, alarmMin, alarmSec);
  lcd.print(buffer);

  // Lógica para fazer o item selecionado piscar
  // (pisca a cada 500ms quando não está no estado IDLE)
  if (estadoAtual != IDLE && (millis() % 1000 < 500)) {
    switch (estadoAtual) {
      case SET_HOURS: lcd.setCursor(7, 0); lcd.print("  "); break;
      case SET_MINUTES: lcd.setCursor(10, 0); lcd.print("  "); break;
      case SET_SECONDS: lcd.setCursor(13, 0); lcd.print("  "); break;
      case SET_ALARM_HOURS: lcd.setCursor(7, 1); lcd.print("  "); break;
      case SET_ALARM_MINUTES: lcd.setCursor(10, 1); lcd.print("  "); break;
      case SET_ALARM_SECONDS: lcd.setCursor(13, 1); lcd.print("  "); break;
    }
  }
}

// Função para ler o estado do botão com debounce
bool botaoPressionado() {
  if (digitalRead(JOY_BUTTON_PIN) == LOW) { // Botão pressionado
    if (millis() - lastButtonPress > BUTTON_DEBOUNCE_INTERVAL) {
      lastButtonPress = millis();
      return true;
    }
  }
  return false;
}

// Função para ler o joystick (cima pra baixo) com debounce
// Retorna: 1 para Cima, -1 para Baixo, 0 para Centro
int lerJoystickY() {
  if (millis() - lastJoystickRead > JOYSTICK_DEBOUNCE_INTERVAL) {
    int valY = analogRead(JOY_Y_PIN);
    lastJoystickRead = millis();
    
    if (valY > 800) return 1;  // Cima
    if (valY < 200) return -1; // Baixo
  }
  return 0; // Centro
}

// --- Leitura do eixo X (Esquerda/Direita) ---
int lerJoystickX() {
  if (millis() - lastJoystickRead1 > JOYSTICK_DEBOUNCE_INTERVAL) {
    int valX = analogRead(JOY_X_PIN);
    lastJoystickRead1 = millis();
    
    if (valX > 800) return 1;   // Direita
    if (valX < 200) return -1;  // Esquerda
  }
  return 0; // Centro
}