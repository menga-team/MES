#include "gpu.h"
#include "second_file.h"

int main() {
        // TODO: investigate
        // for whatever reason the first string will not get indexed correctly, making gpu_print_text not work
        // correctly (possible other functions too). But every string after that seems fine...
        gpu_print_text(FRONT_BUFFER, 0, 0, 7, 0, "BUG");
        gpu_blank(FRONT_BUFFER, 0);
        gpu_print_text(FRONT_BUFFER, 0, 8, 1, 0, "Hello World!");
        gpu_print_text(FRONT_BUFFER, 0, 16, 2, 0, "This is the hello world");
        gpu_print_text(FRONT_BUFFER, 0, 24, 3, 0, "test application!");
        gpu_print_text(FRONT_BUFFER, 0, 32, 4, 0, "Does it work?");
        do_something();
}