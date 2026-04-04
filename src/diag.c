#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "diag.h"

static FILE *diag_trace_file = NULL;

void diag_init(FILE *trace_file) {
    diag_trace_file = trace_file;
}

void diag_shutdown(void) {
    diag_trace_file = NULL;
}

void diag_info(const char *fmt, ...) {
    va_list args;

    if (diag_trace_file == NULL) {
        return;
    }

    va_start(args, fmt);
    vfprintf(diag_trace_file, fmt, args);
    fprintf(diag_trace_file, "\n");
    va_end(args);
}

void diag_errorf(int line, int column, const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "erro:%d:%d: ", line, column);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void diag_syntax_expected(Category expected, const Token *found) {
    diag_errorf(found->line,
                found->column,
                "esperado %s, encontrado %s \"%s\"",
                token_category_name(expected),
                token_category_name(found->category),
                found->lexeme != NULL ? found->lexeme : "");
}
