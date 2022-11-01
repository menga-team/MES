#ifndef MES_MAIN_H
#define MES_MAIN_H

#define SYNC_PIN GPIO13
#define CLOCK_PIN GPIO12
#define DATA_PIN GPIO11

#define SYNC_PORT GPIOB
#define CLOCK_PORT GPIOB
#define DATA_PORT GPIOB

#define CONTROLLER_FREQ 4000

void configure_io(void);

void setup_timers(void);

void wait_for_gpu(void);


#endif //MES_MAIN_H
