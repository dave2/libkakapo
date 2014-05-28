#include "global.h"

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "clock.h"
#include "usart.h"

FILE * other_usart;

int main(void) {
  sysclk_init();
  // Enable interrupts for the USARTS
  sei();

  // Initialise usart_d0 (PD2/PD3 connected to USB FTDI)
  usart_init(usart_d0, 128, 128);
  usart_conf(usart_d0, 9600, 8, none, 1, 0, NULL);
  // because first call to this maps STDIN/STDOUT
  usart_map_stdio(usart_d0);
  usart_run(usart_d0);

  // Now initialise usart_c0 (PC2/PC3)
  usart_init(usart_c0, 128, 128);
  usart_conf(usart_c0, 9600, 8, none, 1, 0, NULL);
  // Grab the file handle so we can use it later
  other_usart = usart_map_stdio(usart_c0);
  usart_run(usart_c0);

  int ch;
  while (1) {
    // Continuously read from "other" USART and print to STDOUT
    while ((ch = fgetc(other_usart)) != EOF) {
      putchar(ch);
    }
    // Continuously read from STDIN and print to "other" USART
    while ((ch = getchar()) != EOF) {
      fputc(ch, other_usart);
      // Also print to STDOUT so we can see what's being sent
      putchar(ch);
    }
  }
}

