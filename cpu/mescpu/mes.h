/* This file is part of MES.
 *
 * MES is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MES is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warrantyp of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MES. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MES_MAIN_H
#define MES_MAIN_H

#include <stdint.h>

#define CODE_EXIT 0
#define CODE_RESTART 1
#define CODE_FREEZEFRAME 2

#define USFR_IS_UNDEFSTR(X) (X & 0b0000000000000001)
#define USFR_IS_INVSTATE(X) (X & 0b0000000000000010)
#define USFR_IS_INVPC(X) (X & 0b0000000000000100)
#define USFR_IS_NOCP(X) (X & 0b0000000000001000)
#define USFR_IS_UNALIGNED(X) (X & 0b0000000100000000)
#define USFR_IS_DIVBYZERO(X) (X & 0b0000001000000000)

#define MMFSR_IS_IACCVIOL(X) (X & 0b00000001)
#define MMFSR_IS_DACCVIOL(X) (X & 0b00000010)
#define MMFSR_IS_MUNSTKERR(X) (X & 0b00001000)
#define MMFSR_IS_MSTKERR(X) (X & 0b00010000)
#define MMFSR_IS_MLSPERR(X) (X & 0b00100000)
#define MMFSR_IS_MMARVALID(X) (X & 0b10000000)

extern const char *GAME_ENTRY;
extern const uint32_t REVISION;

extern char *unknown_symbol;

void configure_io(void);

void clock_peripherals(void);

void invalid_location_get_lot_base(uint32_t adr);

void unrecoverable_error(void);

uint8_t run_game(void *adr);

void handle_hard_fault(uint32_t *stack_frame);

#endif // MES_MAIN_H
