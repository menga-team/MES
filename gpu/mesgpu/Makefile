PROJECT		= mesgpu
OBJS		= main.o
PROGRAMMER	= st-flash
OPENCM3_DIR	:= ../../libopencm3
LDSCRIPT	= ../../utils/stm32f103c8tx.ld
LIBS		= ../../include

# stm32f1 series
LIBNAME		= opencm3_stm32f1
DEFS		+= -DSTM32F1

ARCH_FLAGS	= -mcpu=cortex-m3 -mlittle-endian -mthumb

PREFIX		?= arm-none-eabi
CC		:= $(PREFIX)-gcc
CXX		:= $(PREFIX)-g++
LD		:= $(PREFIX)-gcc
AR		:= $(PREFIX)-ar
AS		:= $(PREFIX)-as
SIZE		:= $(PREFIX)-size
OBJCOPY		:= $(PREFIX)-objcopy
OBJDUMP		:= $(PREFIX)-objdump
GDB		:= $(PREFIX)-gdb
STFLASH		= st-flash
OPT		:= -O2
DEBUG		:= -ggdb3
CSTD		?= -std=c99

TGT_CFLAGS	+= $(OPT) $(CSTD) $(DEBUG)
TGT_CFLAGS	+= $(ARCH_FLAGS)
TGT_CFLAGS	+= -Wextra -Wshadow -Wimplicit-function-declaration
TGT_CFLAGS	+= -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
TGT_CFLAGS	+= -fno-common -ffunction-sections -fdata-sections

TGT_CXXFLAGS	+= $(OPT) $(CXXSTD) $(DEBUG)
TGT_CXXFLAGS	+= $(ARCH_FLAGS)
TGT_CXXFLAGS	+= -Wextra -Wshadow -Wredundant-decls -Weffc++
TGT_CXXFLAGS	+= -fno-common -ffunction-sections -fdata-sections
TGT_CXXFLAGS	+= -std=c++11

DEFS		+= -I$(OPENCM3_DIR)/include
LDFLAGS		+= -L$(OPENCM3_DIR)/lib
LDLIBS		+= -l$(LIBNAME)
LDLIBS		+= -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

# Common flags
TGT_CPPFLAGS	+= -I$(LIBS)
TGT_CPPFLAGS	+= -MD
TGT_CPPFLAGS	+= -Wall -Wundef
TGT_CPPFLAGS	+= $(DEFS)

.SUFFIXES: .elf .bin .hex .srec .list .map .images
.SECONDEXPANSION:
.SECONDARY:

all: elf size
size: $(PROJECT).size
elf: $(PROJECT).elf
bin: $(PROJECT).bin
hex: $(PROJECT).hex
srec: $(PROJECT).srec
list: $(PROJECT).list
flash: $(PROJECT).flash

GENERATED_BINARIES=$(PROJECT).elf $(PROJECT).bin $(PROJECT).hex $(PROJECT).srec $(PROJECT).list $(PROJECT).map

%.bin: %.elf
	$(OBJCOPY) -Obinary $(*).elf $(*).bin

%.hex: %.elf
	$(OBJCOPY) -Oihex $(*).elf $(*).hex

%.srec: %.elf
	$(OBJCOPY) -Osrec $(*).elf $(*).srec

%.list: %.elf
	$(OBJDUMP) -S $(*).elf > $(*).list

%.elf %.map: $(OBJS) $(LDSCRIPT)
	$(LD) $(TGT_LDFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(*).elf

%.o: %.c
	$(CC) $(TGT_CFLAGS) $(CFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $(*).o -c $(*).c

%.o: %.cxx
	$(CXX) $(TGT_CXXFLAGS) $(CXXFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $(*).o -c $(*).cxx

%.o: %.cpp
	$(CXX) $(TGT_CXXFLAGS) $(CXXFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $(*).o -c $(*).cpp

%.size: %.elf
	@echo "Output code size:"
	@$(SIZE) -A -d $(*).elf | egrep 'text|data|bss' | awk ' \
    function human(x) { \
        if (x<1000) {return x} else {x/=1024} \
        s="kMGTEPZY"; \
        while (x>=1000 && length(s)>1) \
            {x/=1024; s=substr(s,2)} \
        return int(x+0.5) substr(s,1,1) \
    } \
	{printf("%10s %8s\n", $$1, human($$2))} \
'

%.flash: %.elf
	$(PROGRAMMER) write $(*).elf 0x8000000

clean:
	$(RM) $(GENERATED_BINARIES) generated.* $(OBJS) $(OBJS:%.o=%.d)

.PHONY: images clean stylecheck styleclean elf bin hex srec list
-include $(OBJS:.o=.d)
