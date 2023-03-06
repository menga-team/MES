PROJECT		= hello-world
CFILES		= $(wildcard src/*.c)
INCLUDES	= -I../../include -I../../cpu/mescpu
MODULE_NAME	= game

PREFIX	?= arm-none-eabi-
CC	= $(PREFIX)gcc
CXX	= $(PREFIX)g++
LD	= $(PREFIX)gcc
OBJCOPY	= $(PREFIX)objcopy
OBJDUMP	= $(PREFIX)objdump

src/%.asset: $(wildcard assets/*)
	for file in $^ ; do \
		base=$$(basename $${file}) ; \
		python asset_packer.py $${file} > src/$${base}.asset ; \
	done

$(MODULE_NAME).bin: $(CFILES) src/%.asset
	udynlink/scripts/mkmodule --name "$(MODULE_NAME)" --includes "$(INCLUDES)" $(CFILES)

$(MODULE_NAME).iso: $(MODULE_NAME).bin
	todo

clean:
	-rm src/*.asset
	-rm src/*.o
	-rm src/*.elf
	-rm src/*.bin
	-rm $(MODULE_NAME).bin
	-rm $(MODULE_NAME).iso

assets: src/%.asset
all: $(MODULE_NAME).bin
.PHONY: all clean
