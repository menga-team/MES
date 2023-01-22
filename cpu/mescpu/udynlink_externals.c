#include <malloc.h>
#include <string.h>
#include "udynlink_externals.h"
#include "stdint.h"
#include "gpu.h"

void *udynlink_external_malloc(size_t size) {
        return malloc(size);
}

void udynlink_external_free(void *p) {
        return free(p);
}

void udynlink_external_vprintf(const char *s, va_list va) {}

uint32_t udynlink_external_resolve_symbol(const char *name) {
        // TODO: This is just temporary for testing
        gpu_print_text(FRONT_BUFFER, 0, 84, 2, 0, name);
        if (strcmp(name, "gpu_print_text") == 0) {
                return (uint32_t) &gpu_print_text;
        }
        else if (strcmp(name, "gpu_blank") == 0) {
                return (uint32_t) &gpu_blank;
        }
        return 0;
}