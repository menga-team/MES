MEMORY {
	rom (rx)      : ORIGIN = 0x8000000, LENGTH = 128K
	ram (xrw)      : ORIGIN = 0x20000000, LENGTH = 20K
}

INCLUDE cortex-m-generic.ld
