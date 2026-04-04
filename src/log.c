#include <stdio.h>
#include <string.h>

#include "log.h"

FILE *log_open_with_extension(const char *source_path, const char *extension, char *out_path, size_t out_size) {
    size_t len;
    const char *dot;

    if (out_size == 0) {
        return NULL;
    }

    strncpy(out_path, source_path, out_size - 1);
    out_path[out_size - 1] = '\0';

    dot = strrchr(out_path, '.');
    if (dot != NULL) {
        out_path[dot - out_path] = '\0';
    }

    len = strlen(out_path);
    if (len + strlen(extension) + 1 > out_size) {
        return NULL;
    }

    strcat(out_path, extension);
    return fopen(out_path, "w");
}
