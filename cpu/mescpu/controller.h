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

#ifndef MES_CONTROLLER_H
#define MES_CONTROLLER_H

#include "controller_internal.h"

/**
 * Gets button state by controller index and button index.
 * @param controller: index of controller
 * @param button: index of button
 *
 * @return: button state
 */
uint16_t controller_get_button_by_controller_and_index(int controller,
                                                       int button);

/**
 * Gets array of button states by controller index
 *
 * @param controller: index of controller
 *
 * @return: array button states
 */
uint16_t *controller_get_buttons(int controller);

/**
 * Gets controller status
 *
 * @param controller: index of controller
 *
 * @return: status of controller
 */
uint16_t controller_get_status(int controller);

/**
 * Gets array of controller statuses
 *
 * @return: array of controller status
 */
uint16_t *controller_get_statuses(void);

#endif // MES_CONTROLLER_H
