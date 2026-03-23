#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "lex.h"
#include "token.h"



static int lex_peek(Lexer *lexer);
static int lex_advance(Lexer *lexer);

static void skip_whitespace(Lexer *lexer);
static void skip_comments(Lexer *lexer);
static void skip_ignored(Lexer *lexer);

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
    int c;

    skip_ignored(lexer);

    c = lex_peek(lexer);

    if (c == EOF) {
        return lex_make_token(sEOF, "", lexer -> line, lexer -> column); // -> acessa um atributo da struct por ponterio 
    }

    if (isalpha(c) || c == '_') {
        return lex_read_identifier_or_keyword(lexer);
    }

    if (isdigit(c)) {
        return lex_read_number(lexer);
    }

    if (c == '"') {
        return lex_read_string(lexer);
    }

    if (c == '\'') {
        return lex_read_char(lexer);
    }

    return lex_read_operator_or_delimiter(lexer);
}

/* =========================
   Implementações internas
   ========================= */

static void skip_ignored(Lexer *lexer) {
    long before;
    long after;

    do {
        before = ftell(lexer->file_pointer);

        skip_whitespace(lexer);
        skip_comments(lexer);

        after = ftell(lexer->file_pointer);
    } while (before != after);
}

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
    int c = lex_peek(lexer);

    if (c != '@') {
        return;
    }

    /* consome o '@' inicial */
    lex_advance(lexer);

    c = lex_peek(lexer);

    if (c == '{') {
        lex_advance(lexer); /* consome '{' */

        while (1) {
            c = lex_advance(lexer);

            if (c == EOF) {
                return;
            }

            if (c == '}') {
                if (lex_peek(lexer) == '@') {
                    lex_advance(lexer); /* consome @ */
                    break;
                }
            }
        }
    }
    else {
        while (1) {
            c = lex_peek(lexer);

            if (c == '\n' || c == EOF) {
                break;
            }

            lex_advance(lexer);
        }
    }
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
    int line = lexer->line;
    int column = lexer->column;
    int c = lex_peek(lexer);
    char aux[2];
    char concat[256];

    concat[0] = '\0';

    while (isdigit(c)) {
        aux[0] = (char)lex_advance(lexer);
        aux[1] = '\0';
        strcat(concat, aux);
        c = lex_peek(lexer);
    }

    return lex_make_token(sCTEINT, concat, line, column);
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