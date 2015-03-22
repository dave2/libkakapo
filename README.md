This is the source code for libkakapo, a set of drivers for the AVR XMEGA
series of chips. The "Kakapo" development board uses such a chip.

This library has been designed so that you can either use it for the
Kakapo development board directly, or link to parts of it from your
own project/board.

More about Kakapo: http://hairy.geek.nz/kakapo

Functions that this library provides:

 * Simple initalisation of a Kakapo board (clock, LEDs)
 * Simplified task scheduling using a run queue with two prio levels
 * Ringbuffer for char-orientated uses
 * Drivers for the following XMEGA hardware modules:
   - System/Perpherial clock configuration
   - SPI (master only)
   - TWI (aka I2C, SMBus; master only)
   - USART (interupt driven, buffered, incl. stdio)
   - ADC (ADCA only)
   - NVM (usersig and serial number only)
   - RTC
   - Timers (Type 0,1 only)
 * Drivers for the following ICs
   - WizNet W5500 (incl. stdio for TCP connections)
 * Most XMEGA chips supported, with automatic detection of resources
   available on the part

Most drivers support hooking their respective interupts with user-provided
functions.

This code is provided under the GNU LGPL version 3. Closed-source projects
may link against this library, but modifications to the library must
be made available under the LGPL to others.
