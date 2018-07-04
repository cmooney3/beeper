#
# Makefile template for ATtiny85
# Derived from AVR Crosspack template
#

DEVICE     = attiny85           # See avr-help for all possible devices
CLOCK      = 1000000            # 1Mhz
PROGRAMMER = -c usbtiny -P usb  # For using Adafruit USBtiny
OBJECTS    = main.o             # Add more objects for each .c file here

# fuse settings:
# use http://www.engbedded.com/fusecalc

# This sets the clock to 1Mhz
FUSES = -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
COMPILE = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE) -std=c11

# symbolic targets:
all: main.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

flash: all
	$(AVRDUDE) -U flash:w:main.hex:i

fuse:
	$(AVRDUDE) $(FUSES)

install: flash fuse

# if you use a bootloader, change the command below appropriately:
load: all
	bootloadHID main.hex

clean:
	rm -f main.hex main.elf $(OBJECTS)

# file targets:
main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS)

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size --format=avr --mcu=$(DEVICE) main.elf

cpp:
	$(COMPILE) -E main.c
