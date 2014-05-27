# Where is the toolchain unpacked?
TOOLBASE  ?= /home/dave2/tmp/avr/avr8-gnu-toolchain-linux_x86
# What MCU do we have on this board?
MCU       ?= atxmega64d4
# Port that Kakapo popped up on
PORT      ?= /dev/ttyUSB1
# Application name 
APP        = timers-pwm
# If using libkakapo, uncomment
LIBKAKAPO  = -lkakapo
#Tools we'll need
CC        = $(TOOLBASE)/bin/avr-gcc
AVRDUDE   = /usr/bin/avrdude
OBJCOPY   = $(TOOLBASE)/bin/avr-objcopy
CFLAGS    = -Os --std=c99 -funroll-loops -funsigned-char -funsigned-bitfields -fpack-struct
CFLAGS   += -fshort-enums -Wstrict-prototypes -Wall -mcall-prologues -I. -I../../
CFLAGS   += -mmcu=$(MCU)
INCLUDE   = -L../../
OBJ       = $(patsubst %.c,%.o,$(wildcard *.c))

build: $(APP).hex

eeprom: $(APP).eep
	$(AVRDUDE) -p $(MCU) -c avr109 -P $(PORT) -b 115200 -U eeprom:w:$(APP).eep

program: $(APP).hex
	$(AVRDUDE) -p $(MCU) -c avr109 -P $(PORT) -b 115200 -U flash:w:$(APP).hex -e

$(APP).hex: $(APP).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(APP).eep : $(APP).elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O ihex $< $@

$(APP).elf: $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@ $(LIBKAKAPO)

%.o: %.c %.h Makefile
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJ) $(APP).hex $(APP).elf $(APP).eep

