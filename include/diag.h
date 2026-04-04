#ifndef DIAG_H
#define DIAG_H

#include <stdarg.h>
#include <stdio.h>

#include "token.h"

void diag_init(FILE *trace_file);
void diag_shutdown(void);

void diag_info(const char *fmt, ...);
void diag_errorf(int line, int column, const char *fmt, ...);
void diag_syntax_expected(Category expected, const Token *found);

#endif
