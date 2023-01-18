PROJECT		= mescpu
CFILES		= main.c timer.c gpu.c sdcard.c
OPENCM3_DIR	:= ../../libopencm3
#DEVICE		= stm32f103c8t6
LDSCRIPT	= ../../utils/stm32f103c8tx.ld
OPENCM3_LIB	= opencm3_stm32f1
OPENCM3_DEFS	= -DSTM32F1
ARCH_FLAGS	= -mcpu=cortex-m3 -mlittle-endian -mthumb

INCLUDES	= -I../../include
BUILD_DIR	?= bin
OPT		?= -Os
CSTD		?= -std=c99

include ../../rules.mk
