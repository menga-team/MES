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
