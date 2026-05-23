#include <stdio.h>
#include <string.h>

#include "log.h"

FILE *log_open_with_extension(const char *source_path, const char *extension, char *out_path, size_t out_size) {
    size_t base_len;
    size_t ext_len;
    const char *dot;

    if (out_size == 0) {
        return NULL;
    }

    dot = strrchr(source_path, '.');
    if (dot != NULL) {
        base_len = (size_t)(dot - source_path);
    } else {
        base_len = strlen(source_path);
    }

    ext_len = strlen(extension);
    if (base_len + ext_len + 1 > out_size) {
        return NULL;
    }

    memcpy(out_path, source_path, base_len);
    memcpy(out_path + base_len, extension, ext_len + 1);
    return fopen(out_path, "w");
}
