#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
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

    if (lexeme != NULL) {
        token.lexeme = malloc(strlen(lexeme) + 1);
        if (token.lexeme != NULL) {
            strcpy(token.lexeme, lexeme);
        }
    } else {
        token.lexeme = NULL;
    }

    token.line = line;
    token.column = column;

    return token;
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
    int line = lexer->line;
    int column = lexer->column;
    int c;
    char aux[2];
    char concat[256];

    concat[0] = '\0';

    lex_advance(lexer); // consome as aspas

    c = lex_peek(lexer);

    while (c != '"' && c != EOF && c != '\n') {
        aux[0] = (char)lex_advance(lexer);
        aux[1] = '\0';
        strcat(concat, aux);
        c = lex_peek(lexer);
    }

    if (c != '"') {
        return lex_make_token(sERROR, "", line, column);
    }

    lex_advance(lexer);

    return lex_make_token(sSTRING, concat, line, column);
}

static Token lex_read_char(Lexer *lexer) {
    int line = lexer->line;
    int column = lexer->column;
    int c;
    char aux[2];

    lex_advance(lexer); /* consome aspas simples de abertura */

    c = lex_advance(lexer);
    if (c == EOF || c == '\n' || c == '\'') {
        return lex_make_token(sERROR, "", line, column);
    }

    aux[0] = (char)c;
    aux[1] = '\0';

    c = lex_peek(lexer);
    if (c != '\'') {
        return lex_make_token(sERROR, "", line, column);
    }

    lex_advance(lexer); /* consome aspas simples de fechamento */

    return lex_make_token(sCTECHAR, aux, line, column);
}

static Token lex_read_operator_or_delimiter(Lexer *lexer) {
    int line = lexer->line;
    int column = lexer->column;
    int c = lex_peek(lexer);

    switch (c) {
        case ':':
            lex_advance(lexer);
            if (lex_peek(lexer) == '=') {
                lex_advance(lexer);
                return lex_make_token(sATRIB, ":=", line, column);
            }
            return lex_make_token(sCOLON, ":", line, column);

        case '<':
            lex_advance(lexer);
            if (lex_peek(lexer) == '=') {
                lex_advance(lexer);
                return lex_make_token(sMENORIG, "<=", line, column);
            }
            if (lex_peek(lexer) == '>') {
                lex_advance(lexer);
                return lex_make_token(sDIFERENTE, "<>", line, column);
            }
            return lex_make_token(sMENOR, "<", line, column);

        case '>':
            lex_advance(lexer);
            if (lex_peek(lexer) == '=') {
                lex_advance(lexer);
                return lex_make_token(sMAIORIG, ">=", line, column);
            }
            return lex_make_token(sMAIOR, ">", line, column);

        case '.':
            lex_advance(lexer);
            if (lex_peek(lexer) == '.') {
                lex_advance(lexer);
                return lex_make_token(sPTOPTO, "..", line, column);
            }
            return lex_make_token(sERROR, ".", line, column);

        case '=':
            lex_advance(lexer);
            if (lex_peek(lexer) == '>') {
                lex_advance(lexer);
                return lex_make_token(sIMPLIC, "=>", line, column);
            }
            return lex_make_token(sIGUAL, "=", line, column);

        case '+':
            lex_advance(lexer);
            return lex_make_token(sSOMA, "+", line, column);

        case '-':
            lex_advance(lexer);
            return lex_make_token(sSUBRAT, "-", line, column);

        case '*':
            lex_advance(lexer);
            return lex_make_token(sMULT, "*", line, column);

        case '/':
            lex_advance(lexer);
            return lex_make_token(sDIV, "/", line, column);

        case '^':
            lex_advance(lexer);
            return lex_make_token(sAND, "^", line, column);

        case 'v':
            lex_advance(lexer);
            return lex_make_token(sOR, "v", line, column);

        case '~':
            lex_advance(lexer);
            return lex_make_token(sNEG, "~", line, column);

        case '(':
            lex_advance(lexer);
            return lex_make_token(sLPAR, "(", line, column);

        case ')':
            lex_advance(lexer);
            return lex_make_token(sRPAR, ")", line, column);

        case '[':
            lex_advance(lexer);
            return lex_make_token(sLBRACK, "[", line, column);

        case ']':
            lex_advance(lexer);
            return lex_make_token(sRBRACK, "]", line, column);

        case ',':
            lex_advance(lexer);
            return lex_make_token(sCOMMA, ",", line, column);

        case ';':
            lex_advance(lexer);
            return lex_make_token(sSEMI, ";", line, column);

        default:
            lex_advance(lexer);
            return lex_make_token(sERROR, "", line, column);
    }
}

static enum Category keyword_or_ident(const char *lexeme) {
    if (strcmp(lexeme, "if") == 0) return sIF;
    if (strcmp(lexeme, "int") == 0) return sINT;
    if (strcmp(lexeme, "module") == 0) return sMODULE;
    if (strcmp(lexeme, "proc") == 0) return sPROC;
    if (strcmp(lexeme, "else") == 0) return sELSE;
    if (strcmp(lexeme, "v") == 0) return sOR;
    if (strcmp(lexeme, "start") == 0) return sSTART;
    if (strcmp(lexeme, "end") == 0) return sEND;
    if (strcmp(lexeme, "then") == 0) return sTHEN;

    return sIDENTIF;
}

static Token lex_read_identifier_or_keyword(Lexer *lexer) {
    int line = lexer->line;
    int column = lexer->column;
    int c = lex_peek(lexer);
    char aux[2];
    char concat[256];

    concat[0] = '\0';

    while (isalpha(c) || isdigit(c) || c == '_') {
        aux[0] = (char)lex_advance(lexer);
        aux[1] = '\0';
        strcat(concat, aux);
        c = lex_peek(lexer);
    }

    enum Category category = keyword_or_ident(concat);
    return lex_make_token(category, concat, line, column);
}