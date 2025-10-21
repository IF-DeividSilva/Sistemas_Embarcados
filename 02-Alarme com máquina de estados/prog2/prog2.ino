// ==========================================================
// BIBLIOTECAS
// ==========================================================
#include <LiquidCrystal.h>

#define USE_TIMER_1 true
#include "TimerInterrupt.h"

// ==========================================================
// CONFIGURAÇÕES
// ==========================================================
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

const int BUZZER_PIN = 48;

// ==========================================================
// CLASSES
// ==========================================================

class Botao {
  int pin;
  unsigned long lastPress = 0;
  const long debounce = 250;
public:
  Botao(int p) : pin(p) { pinMode(pin, INPUT_PULLUP); }

  bool pressionado() {
    if (digitalRead(pin) == LOW) {
      if (millis() - lastPress > debounce) {
        lastPress = millis();
        return true;
      }
    }
    return false;
  }
};

class Joystick {
  int pinX, pinY;
  unsigned long lastReadX = 0, lastReadY = 0;
  const long debounce = 150;
public:
  Joystick(int x, int y) : pinX(x), pinY(y) {}

  int lerX() {
    if (millis() - lastReadX > debounce) {
      int valX = analogRead(pinX);
      lastReadX = millis();
      if (valX > 800) return 1;
      if (valX < 200) return -1;
    }
    return 0;
  }

  int lerY() {
    if (millis() - lastReadY > debounce) {
      int valY = analogRead(pinY);
      lastReadY = millis();
      if (valY > 800) return 1;
      if (valY < 200) return -1;
    }
    return 0;
  }
};

class Relogio {
public:
  int hora = 0, minuto = 0, segundo = 0;

  void tick() {
    segundo++;
    if (segundo >= 60) { segundo = 0; minuto++; }
    if (minuto >= 60) { minuto = 0; hora++; }
    if (hora >= 24) hora = 0;
  }

  void ajustar(int h, int m, int s) {
    hora = h; minuto = m; segundo = s;
  }
};

class Sirene {
  int pin;
public:
  Sirene(int p) : pin(p) { pinMode(pin, OUTPUT); }
  void tocar() { tone(pin, 1000); }
  void parar() { noTone(pin); }
};

class TimerRelogio {
public:
  static void iniciar(void (*callback)()) {
    ITimer1.init();
    ITimer1.attachInterruptInterval(1000, callback);
  }
};

// ==========================================================
// OBJETOS
// ==========================================================
Botao botao(A2);
Joystick joystick(A0, A1);
Relogio relogio;
Relogio alarme;
Sirene sirene(BUZZER_PIN);

// ==========================================================
// VARIÁVEIS DE ESTADO
// ==========================================================
enum Estado {
  IDLE,
  SET_HOURS, SET_MINUTES, SET_SECONDS,
  SET_ALARM_HOURS, SET_ALARM_MINUTES, SET_ALARM_SECONDS,
  PLAY_ALARM
};
Estado estadoAtual = IDLE;

bool alarmeAtivado = true;
bool alarmeDisparado = false;

// ==========================================================
// PROTÓTIPOS
// ==========================================================
void maquinaDeEstados();
void updateLCD();
void TimerHandler();

// ==========================================================
// SETUP
// ==========================================================
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  TimerRelogio::iniciar(TimerHandler);
}

// ==========================================================
// LOOP
// ==========================================================
void loop() {
  maquinaDeEstados();
  updateLCD();
}

// ==========================================================
// TIMER
// ==========================================================
void TimerHandler() {
  relogio.tick();
}

// ==========================================================
// MÁQUINA DE ESTADOS
// ==========================================================
void maquinaDeEstados() {
  int joyX = joystick.lerX();
  int joyY = joystick.lerY();

  switch (estadoAtual) {
    case IDLE:
      if (botao.pressionado()) estadoAtual = SET_HOURS;
      if (alarmeAtivado && relogio.hora == alarme.hora &&
          relogio.minuto == alarme.minuto &&
          relogio.segundo == alarme.segundo && !alarmeDisparado) {
        estadoAtual = PLAY_ALARM;
        alarmeDisparado = true;
      }
      if (relogio.segundo != alarme.segundo ||
          relogio.minuto != alarme.minuto ||
          relogio.hora != alarme.hora) alarmeDisparado = false;
      break;

    case SET_HOURS:
      if (joyY != 0) {
        relogio.hora += joyY;
        if (relogio.hora > 23) relogio.hora = 0;
        if (relogio.hora < 0) relogio.hora = 23;
      }
      if (botao.pressionado()) estadoAtual = SET_ALARM_HOURS;
      if (joyX == 1) estadoAtual = SET_MINUTES;
      break;

    case SET_MINUTES:
      if (joyY != 0) {
        relogio.minuto += joyY;
        if (relogio.minuto > 59) relogio.minuto = 0;
        if (relogio.minuto < 0) relogio.minuto = 59;
      }
      if (joyX == -1) estadoAtual = SET_HOURS;
      if (joyX == 1) estadoAtual = SET_SECONDS;
      break;

    case SET_SECONDS:
      if (joyY != 0) {
        relogio.segundo += joyY;
        if (relogio.segundo > 59) relogio.segundo = 0;
        if (relogio.segundo < 0) relogio.segundo = 59;
      }
      if (joyX == -1) estadoAtual = SET_MINUTES;
      if (joyX == 1) estadoAtual = IDLE;
      break;

    case SET_ALARM_HOURS:
      if (joyY != 0) {
        alarme.hora += joyY;
        if (alarme.hora > 23) alarme.hora = 0;
        if (alarme.hora < 0) alarme.hora = 23;
      }
      if (joyX == 1) estadoAtual = SET_ALARM_MINUTES;
      break;

    case SET_ALARM_MINUTES:
      if (joyY != 0) {
        alarme.minuto += joyY;
        if (alarme.minuto > 59) alarme.minuto = 0;
        if (alarme.minuto < 0) alarme.minuto = 59;
      }
      if (joyX == -1) estadoAtual = SET_ALARM_HOURS;
      if (joyX == 1) estadoAtual = SET_ALARM_SECONDS;
      break;

    case SET_ALARM_SECONDS:
      if (joyY != 0) {
        alarme.segundo += joyY;
        if (alarme.segundo > 59) alarme.segundo = 0;
        if (alarme.segundo < 0) alarme.segundo = 59;
      }
      if (joyX == -1) estadoAtual = SET_ALARM_MINUTES;
      if (joyX == 1) estadoAtual = IDLE;
      break;

    case PLAY_ALARM:
      sirene.tocar();
      if (botao.pressionado()) {
        sirene.parar();
        estadoAtual = IDLE;
      }
      break;
  }
}

// ==========================================================
// LCD
// ==========================================================
void updateLCD() {
  char buffer[17];
  lcd.setCursor(0, 0);
  sprintf(buffer, "Hora:  %02d:%02d:%02d", relogio.hora, relogio.minuto, relogio.segundo);
  lcd.print(buffer);

  lcd.setCursor(0, 1);
  sprintf(buffer, "Alarme:%02d:%02d:%02d", alarme.hora, alarme.minuto, alarme.segundo);
  lcd.print(buffer);

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
