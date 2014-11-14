# Build the libkakapo.a archive containing all the drivers

# Where is the toolchain unpacked?
TOOLBASE  ?= /usr/local/share/avr8-gnu-toolchain-linux_x86_64
# What MCU do we have on this board?
MCU       ?= atxmega64d4
#Tools we'll need
CC        = $(TOOLBASE)/bin/avr-gcc
AR        = $(TOOLBASE)/bin/avr-ar
RANLIB    = $(TOOLBASE)/bin/avr-ranlib
CFLAGS    = -Os --std=c99 -funroll-loops -funsigned-char -funsigned-bitfields -fpack-struct
CFLAGS   += -fshort-enums -Wstrict-prototypes -Wall -mcall-prologues -I.
CFLAGS   += -mmcu=$(MCU)
ifdef F_CPU
CFLAGS	 += -DF_CPU=$(F_CPU)
endif

OBJ += adc.o ringbuffer.o spi.o timer.o usart.o twi.o clock.o rtc.o wdt.o net_w5500.o nvm.o kakapo.o

libkakapo.a : $(OBJ) Makefile
	$(AR) cr libkakapo.a $(OBJ)
	$(RANLIB) libkakapo.a

%.o : %.c %.h Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f $(OBJ) libkakapo.a

