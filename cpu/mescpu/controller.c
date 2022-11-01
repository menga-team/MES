#include <stdint.h>
#include "controller.h"

uint16_t active_controller[4] = {0, 0, 0, 0};
uint16_t buttons[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int counter = 0;

void tim1_cc_isr(void) {
        uint16_t data_bit = gpio_get(data_port, data_pin);
        uint16_t sync_bit = gpio_get(sync_port, sync_pin);
        if (counter == 0) {
                if (sync_bit) {
                        counter++;
                }
        }
        else if (counter < 5) {
                active_controller[counter-1] = sync_bit;
        }
        else {
                if (sync_bit) {
                        counter = 1;
                }
        }
        if (counter) {
                buttons[counter-1] = data_bit;
                counter++;
                if (counter > 31) {

                }
        }
}
/*
 * gets button state by controller index and button index
 *
 * @param controller: index of controller
 * @param button: index of button
 *
 * @return: button state
 */
static uint16_t get_button_by_contorller_and_index(int controller, int button) {
        return buttons[(controller*4) + button];
}

/*
 * return a copy of the button state by controller index and button index
 *
 * @param controller: index of controller
 * @param button: index of button
 *
 * @return: copy of button state
 */
static uint16_t get_button_by_contorller_and_index_copy(int controller, int button) {
        uint16_t temp = buttons[(controller*4) + button];
        return temp;
}

/*
 * gets button state by button index
 *
 * @param button: index of button
 *
 * @return: button state
 */
static uint16_t get_button_by_index(int button) {
        return buttons[button];
}

/*
 * return a copy of the button state by button index
 *
 * @param button: index of button
 *
 * @return: copy of button state
 */
static uint16_t get_button_by_index_copy(int button) {
        uint16_t temp = buttons[button];
        return temp;
}

/*
 * gets array of button states by controller index
 *
 * @param controller: index of controller
 *
 * @return: array button states
 */
static uint16_t * get_buttons_by_controller(int controller) {
        uint16_t * subset[8] = {0, 0, 0, 0, 0, 0, 0, 0 };
        for(int i=0; i<8; i++)
                subset[i] = &buttons[(controller*4) + i];
        return subset;
}

/*
 * gets copy of array of button states by controller index
 *
 * @param controller: index of controller
 *
 * @return: copy of array button states
 */
static uint16_t * get_buttons_by_controller_copy(int controller) {
        uint16_t subset[8] = {0, 0, 0, 0, 0, 0, 0, 0 };
        for(int i=0; i<8; i++)
                subset[i] = buttons[(controller*4) + i];
        return subset;
}

/*
 * gets controller status
 *
 * @param controller: index of controller
 *
 * @return: status of controller
 */
static uint16_t get_controller_status(int controller) {
        return active_controller[controller];
}

/*
 * gets copy of controller status
 *
 * @param controller: index of controller
 *
 * @return: copy of status of controller
 */
static uint16_t get_controller_status_copy(int controller) {
        uint16_t temp = active_controller[controller];
        return temp;
}

/*
 * gets array of controller statuses
 *
 * @return: array of controller status
 */
static uint16_t * get_controller_statuses() {
        return active_controller;
}

/*
 * gets copy of array of controller statuses
 *
 * @return: copy of array of controller status
 */
static uint16_t * get_controller_statuses_copy() {
        uint16_t subset[4];
        memcpy(subset, active_controller, sizeof(uint16_t)*4);
        return subset;
}
