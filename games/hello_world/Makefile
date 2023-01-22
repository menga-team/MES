PROJECT		= hello-world
CFILES		= src/main.c src/second_file.c
INCLUDES	= -I../../include -I../../cpu/mescpu
MODULE_NAME	= game

PREFIX	?= arm-none-eabi-
CC	= $(PREFIX)gcc
CXX	= $(PREFIX)g++
LD	= $(PREFIX)gcc
OBJCOPY	= $(PREFIX)objcopy
OBJDUMP	= $(PREFIX)objdump

%.bin: $(CFILES)
	udynlink/scripts/mkmodule --name "$(MODULE_NAME)" --includes "$(INCLUDES)" $(CFILES)

$(MODULE_NAME).iso: %bin:
	todo

clean:
	-rm src/*.o
	-rm src/*.elf
	-rm src/*.bin
	rm $(MODULE_NAME).bin

all: $(MODULE_NAME).bin
.PHONY: all clean