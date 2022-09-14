MEMORY {
	FLASH (rx)      : ORIGIN = 0x8000000, LENGTH = 64K
	RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 20K
}

INCLUDE cortex-m-generic.ld
