/*
 * Example of how to use the avr-libc EEPROM functions;
 * http://www.nongnu.org/avr-libc/user-manual/group__avr__eeprom.html
 *
*/

#define F_CPU 32000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "clock.h"

#define EXPECTED_MAGIC 27
#define STRING_LEN 13

uint8_t eeprom_magic_byte EEMEM = 0;
uint16_t eeprom_big EEMEM = 0;
uint32_t eeprom_bigger EEMEM = 0;
char eeprom_string[STRING_LEN] EEMEM = "";

uint8_t magic_byte = 0;
uint16_t big = 0;
uint32_t bigger = 0;
char string[STRING_LEN] = "";

int main(void) {
  kakapo_init();
  sei();

  // configure the usart_d0 (attached to USB) for 9600,8,N,1 to
  // show debugging info, see usart examples for explanation
  usart_init(usart_d0, 128, 128);
  usart_conf(usart_d0, 9600, 8, none, 1, 0, NULL);
  usart_map_stdio(usart_d0);
  usart_run(usart_d0);

  // wait for EEPROM to be ready
  eeprom_busy_wait();

  // Delay to give time for you to open a terminal and see the output
  _delay_ms(2000);

  // read magic byte from EEPROM to see if we've ever run before
  magic_byte = eeprom_read_byte(&eeprom_magic_byte); // single byte

  if (magic_byte != EXPECTED_MAGIC) {
    puts("No magic found, setting values to defaults.\r");
    puts("Reset your Kakapo to run again.\r");

    magic_byte = EXPECTED_MAGIC;
    big = 12345;
    bigger = 1234567890UL;
    strncpy(string, "Hello World!\0", STRING_LEN);

    // update values to EEPROM
    // the _update_ functions will test whether the existing EEPROM value is the same
    // before writing to save flash wear, there are also _write_ functions which do not
    eeprom_update_byte(&eeprom_magic_byte, magic_byte);
    eeprom_update_word(&eeprom_big, big);
    eeprom_update_dword(&eeprom_bigger, bigger);
    eeprom_update_block((void*)&string, (void*)&eeprom_string, STRING_LEN);
  } else {
    puts("Magic found, reading values from EEPROM;\r\n");

    big = eeprom_read_word(&eeprom_big); // word
    bigger = eeprom_read_dword(&eeprom_bigger); // long integer
    eeprom_read_block((void*)&string, (void*)&eeprom_string, STRING_LEN); // char array

    _delay_ms(100);
    printf("magic_byte = %u\r\n", magic_byte);
    printf("big = %u\r\n", big);
    printf("bigger = %lu\r\n", bigger);
    printf("string = %s\r\n", string);
  }

  puts("========================\r\n");
  _delay_ms(100); // let the characters go out the usart

  return 0;
}

