/*
 * An example of several tasks running, using sched_simple.
 * Task A echos USART characters.
 * Task B blinks an LED when a timer expires.
*/

#define F_CPU 32000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include <util/delay.h>
#include "usart.h"
#include "kakapo.h"
#include "timer.h"
#include "sched_simple.h"

/* our two tasks, prototypes */

/* a will echo usart characters */
void task_a(task_t *task);
/* b will toggle an LED when called, which we'll trigger from a timer */
void task_b(task_t *task);

/* a prototype to hook the USART recieve event */
void usart_rxhook(uint8_t c);
/* a prototype to hook the timer overflow */
void timer_ovfhook(void);

task_t *led_task;
task_t *usart_task;

/* used as a counter to cut down how often that 1ms timer fires the task */
uint16_t count;

int main(void) {
    kakapo_init();

    sei();

    // configure the usart_d0 (attached to USB) for 9600,8,N,1 to
    // show debugging info, see usart examples for explanation
    usart_init(usart_d0, 128, 128);
    usart_conf(usart_d0, 921600, 8, none, 1, 0, &usart_rxhook);
    usart_map_stdio(usart_d0);
    usart_run(usart_d0);

    // this is required to force FTDI chip to resync
    putchar(0);
    _delay_ms(1);

    printf("scheduler test\r\n");

    count = 0;

    /* set up the timer to fire now and then */
    timer_init(timer_c0,timer_norm,32000,NULL,&timer_ovfhook);
    timer_clk(timer_c0,timer_perdiv1);

    sched_simple_init();
    led_task = sched_create(&task_b);
    usart_task = sched_create(&task_a);

    while (1) {
        /* don't bother sleeping, just constantly try to run something */
        sched_simple();
    }

    return 0;
}

void task_a(task_t *task) {
    /* we get told to run when a character is recieved */
    putchar(getchar());
    return;
}

void task_b(task_t *task) {
    /* very simply, toggle one of the LEDs every time we're invoked */
    PORTE.OUTTGL = PIN3_bm; /* yellow LED */
    return;
}

void usart_rxhook(uint8_t c) {
    sched_run(usart_task,sched_later);
}

void timer_ovfhook(void) {
    count++;
    if (count == 1000) { // 100ms between toggles
        count = 0;
        sched_run(led_task,sched_later);
    }
}
