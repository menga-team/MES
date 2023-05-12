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

#include "data.h"
#include "sdcard.h"

uint8_t *data_read_block(uint32_t index) {
    uint8_t *block = malloc(SD_SECTOR_SIZE);
    bool match;
    do {
        match = sdcard_read_sector(index + DATA_SECTOR_START_OFFSET, block);
    } while (!match);
    return block;
}

void data_write_block(uint32_t index, uint8_t *block) {
    sdcard_write_sector(index + DATA_SECTOR_START_OFFSET, block);
}
