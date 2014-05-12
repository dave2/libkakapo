# Build the libkakapo.a archive containing all the drivers

CC=avr-gcc
AR=avr-ar
RANLIB=avr-ranlib
CFLAGS=-Os --std=c99 -funroll-loops -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wstrict-prototypes -Wall -mcall-prologues -I.
CFLAGS += -mmcu=atxmega64d4

OBJ += adc.o ringbuffer.o spi.o timer.o usart.o twi.o clock.o

libkakapo.a : $(OBJ) Makefile
	$(AR) cr libkakapo.a $(OBJ)
	$(RANLIB) libkakapo.a

%.o : %.c %.h Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f $(OBJ) libkakapo.a
