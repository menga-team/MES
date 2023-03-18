#ifndef MES_MAIN_H
#define MES_MAIN_H

#include <stdint.h>

#define USFR_IS_UNDEFSTR(X) (X & 0b0000000000000001)
#define USFR_IS_INVSTATE(X) (X & 0b0000000000000010)
#define USFR_IS_INVPC(X) (X & 0b0000000000000100)
#define USFR_IS_NOCP(X) (X & 0b0000000000001000)
#define USFR_IS_UNALIGNED(X) (X & 0b0000000100000000)
#define USFR_IS_DIVBYZERO(X) (X & 0b0000001000000000)

extern const char *GAME_ENTRY;
extern const uint32_t REVISION;

extern char *unknown_symbol;

void configure_io(void);

void clock_peripherals(void);

void invalid_location_get_lot_base(uint32_t adr);

void unrecoverable_error(void);

uint8_t run_game(void *adr);

#endif // MES_MAIN_H
