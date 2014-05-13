
#define F_CPU 2000000
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
 PORTE.DIRSET = PIN2_bm;
 while (1) {
  PORTE.OUTSET = PIN2_bm;
  _delay_ms(500);
  PORTE.OUTCLR = PIN2_bm;
  _delay_ms(500);
 }
 return 0;
}
