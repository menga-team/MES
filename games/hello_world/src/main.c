#include "gpu.h"
#include "second_file.h"

static const char *GLOBAL_VAR = "GLOBAL";

uint8_t start(void) {
    gpu_blank(FRONT_BUFFER, 0);
    gpu_print_text(FRONT_BUFFER, 0, 0, 7, 0, "I am .LC0 from main.o!");
    gpu_print_text(FRONT_BUFFER, 0, 8, 1, 0, "Hello World!");
    gpu_print_text(FRONT_BUFFER, 0, 16, 2, 0, "This is the hello world");
    gpu_print_text(FRONT_BUFFER, 0, 24, 3, 0, "test application!");
    gpu_print_text(FRONT_BUFFER, 0, 32, 4, 0, "Does it work?");
    gpu_print_text(FRONT_BUFFER, 0, 40, 5, 0, GLOBAL_VAR);
    do_something();
    return 0;
}
