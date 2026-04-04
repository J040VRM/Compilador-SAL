#ifndef LEX_H
#define LEX_H

#include <stdio.h>

#include "token.h"

typedef struct {
    FILE *file;
    FILE *token_log;
    int line;
    int column;
} Lexer;

void lex_init(Lexer *lexer, FILE *file, FILE *token_log);
void lex_destroy(Lexer *lexer);
Token lex_next(Lexer *lexer);

#endif
