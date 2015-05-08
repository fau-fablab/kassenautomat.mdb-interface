#include <avr/pgmspace.h>
void __attribute__((noreturn)) fatal_real(const char * PROGMEM str, uint8_t d);
#define FATAL(x) fatal_real(PSTR(x),0)
#define FATAL_h(x,d) fatal_real(PSTR(x),d)

const float TICKS_PER_MS=5.8; // how often is task_XYZ() called per milisecond