# Where is the toolchain unpacked?
TOOLBASE  = /usr/local/share/avr8-gnu-toolchain-linux_x86_64
# What MCU do we have on this board?
MCU       = atxmega64d4
# Port that Kakapo popped up on
PORT      = /dev/ttyUSB2
# Application name 
APP       = usart-passthru
# If using libkakapo, uncomment
LIBKAKAPO = -lkakapo
#Tools we'll need
CC        = $(TOOLBASE)/bin/avr-gcc
AVRDUDE   = /usr/bin/avrdude 
OBJCOPY   = $(TOOLBASE)/bin/avr-objcopy
CFLAGS    = -Os --std=c99 -funroll-loops -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wstrict-prototypes -Wall -mcall-prologues -I. -I../../
CFLAGS   += -mmcu=$(MCU)
INCLUDE   = -L../../
OBJ       = $(patsubst %.c,%.o,$(wildcard *.c))

build: $(APP).hex

program: $(APP).hex
	$(AVRDUDE) -p $(MCU) -c avr109 -P $(PORT) -b 115200 -U flash:w:$(APP).hex -e

$(APP).hex: $(APP).elf
	$(OBJCOPY) -O ihex $< $@ 

$(APP).elf: $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@ $(LIBKAKAPO)

%.o: %.c %.h Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJ) $(APP).hex $(APP).elf

