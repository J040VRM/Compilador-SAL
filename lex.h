#ifndef LEX_H
#define LEX_H

#include <stdio.h>
#include "token.h"

typedef struct {
    FILE *file_pointer;
    int line;
    int column;
} Lexer;

Lexer lex_init(FILE *file_pointer);
Token lex_next(Lexer *lexer);
void lex_destroy(Lexer *lexer);

#endif