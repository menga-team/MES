#ifndef MES_CONTROLLER_H
#define MES_CONTROLLER_H

#define SYNC_PIN GPIO13
#define CLOCK_PIN GPIO_TIM1_CH1 // PA8
#define DATA_PIN GPIO11

#define SYNC_PORT GPIOB
#define CLOCK_PORT GPIOA
#define DATA_PORT GPIOB

#define CONTROLLER_FREQ 4000

/**
 * Initialize timers needed for controllers.
 */
void controller_setup_timers(void);

/**
 * Gets button state by controller index and button index.
 * @param controller: index of controller
 * @param button: index of button
 *
 * @return: button state
 */
uint16_t controller_get_button_by_controller_and_index(int controller, int button);

/**
 * Return a copy of the button state by controller index and button index
 *
 * @param controller: index of controller
 * @param button: index of button
 *
 * @return: copy of button state
 */
uint16_t controller_get_button_copy_by_controller_and_index(int controller, int button);


/**
 * Gets button state by button index
 *
 * @param button: index of button
 *
 * @return: button state
 */
uint16_t controller_get_button_by_index(int button);

/**
 * Return a copy of the button state by button index
 *
 * @param button: index of button
 *
 * @return: copy of button state
 */
uint16_t controller_get_button_copy_by_index(int button);

/**
 * Gets array of button states by controller index
 *
 * @param controller: index of controller
 *
 * @return: array button states
 */
uint16_t **controller_get_buttons(int controller);

/**
 * Gets copy of array of button states by controller index
 *
 * @param controller: index of controller
 *
 * @return: copy of array button states
 */
uint16_t *controller_get_buttons_by_copy(int controller);

/**
 * Gets controller status
 *
 * @param controller: index of controller
 *
 * @return: status of controller
 */
uint16_t controller_get_status(int controller);

/**
 * Gets copy of controller status
 *
 * @param controller: index of controller
 *
 * @return: copy of status of controller
 */
uint16_t controller_get_status_copy(int controller);

/**
 * Gets array of controller statuses
 *
 * @return: array of controller status
 */
uint16_t *controller_get_statuses(void);

/**
 * Gets copy of array of controller statuses
 *
 * @return: copy of array of controller status
 */
uint16_t *controller_get_statuses_copy(void);

/**
 * Configures PINs for communicating with controllers
 */
void controller_configure_io(void);

#endif //MES_CONTROLLER_H
