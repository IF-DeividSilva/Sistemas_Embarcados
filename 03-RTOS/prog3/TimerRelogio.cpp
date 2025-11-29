#include "TimerRelogio.h"
#define USE_TIMER_1 true
#include "TimerInterrupt.h"

void TimerRelogio::iniciar(void (*callback)()) {
  ITimer1.init();
  ITimer1.attachInterruptInterval(1000, callback);
}