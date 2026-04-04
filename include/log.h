#ifndef LOG_H
#define LOG_H

#include <stddef.h>
#include <stdio.h>

FILE *log_open_with_extension(const char *source_path, const char *extension, char *out_path, size_t out_size);

#endif
