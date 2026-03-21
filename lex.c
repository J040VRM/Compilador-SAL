#include <stdio.h>
#include <ctype.h>
#include "lex.h"
#include "token.h"



static int lex_peek(Lexer *lexer);
static int lex_advance(Lexer *lexer);

static void skip_whitespace(Lexer *lexer);
static void skip_comments(Lexer *lexer);

static Token lex_make_token(enum Category category, const char *lexeme, int line, int column);

static Token lex_read_identifier_or_keyword(Lexer *lexer);
static Token lex_read_number(Lexer *lexer);
static Token lex_read_string(Lexer *lexer);
static Token lex_read_char(Lexer *lexer);
static Token lex_read_operator_or_delimiter(Lexer *lexer);



Lexer lex_init(FILE *file_pointer) {
    Lexer lexer;
    lexer.file_pointer = file_pointer;
    lexer.line = 1;
    lexer.column = 1;
    return lexer;
}

void lex_destroy(Lexer *lexer) {
    (void)lexer;
}

Token lex_next(Lexer *lexer) {
    skip_whitespace(lexer);
    skip_comments(lexer);
    skip_whitespace(lexer);

    /* Aqui depois você vai:
       1. verificar EOF
       2. olhar o próximo caractere
       3. decidir qual função chamar
    */

    return lex_make_token(sEOF, "", lexer->line, lexer->column);
}

/* =========================
   Implementações internas
   ========================= */

static int lex_peek(Lexer *lexer) {
    int c = fgetc(lexer->file_pointer);
    if (c != EOF) {
        ungetc(c, lexer->file_pointer);
    }
    return c;
}

static int lex_advance(Lexer *lexer) {
    int c = fgetc(lexer->file_pointer);

    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else if (c != EOF) {
        lexer->column++;
    }

    return c;
}

static void skip_whitespace(Lexer *lexer) {
    int c = lex_peek(lexer);

    while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        lex_advance(lexer);
        c = lex_peek(lexer);
    }
}

static void skip_comments(Lexer *lexer) {
    /* Aqui depois você vai tratar:
       @ comentário de linha
       @{ ... }@ comentário de bloco
    */
    (void)lexer;
}

static Token lex_make_token(enum Category category, const char *lexeme, int line, int column) {
    Token token;
    token.category = category;
    token.lexeme = NULL; /* depois você decide como copiar o lexema */
    token.line = line;
    token.column = column;

    (void)lexeme;
    return token;
}

static Token lex_read_identifier_or_keyword(Lexer *lexer) {
    (void)lexer;
    return lex_make_token(sEOF, "", 0, 0);
}

static Token lex_read_number(Lexer *lexer) {
    (void)lexer;
    return lex_make_token(sEOF, "", 0, 0);
}

static Token lex_read_string(Lexer *lexer) {
    (void)lexer;
    return lex_make_token(sEOF, "", 0, 0);
}

static Token lex_read_char(Lexer *lexer) {
    (void)lexer;
    return lex_make_token(sEOF, "", 0, 0);
}

static Token lex_read_operator_or_delimiter(Lexer *lexer) {
    (void)lexer;
    return lex_make_token(sEOF, "", 0, 0);
}