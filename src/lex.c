#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "diag.h"
#include "lex.h"

#define LEXEME_CAP 256

typedef struct {
    const char *keyword;
    Category category;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"bool", sBOOL},
    {"char", sCHAR},
    {"do", sDO},
    {"else", sELSE},
    {"end", sEND},
    {"false", sFALSE},
    {"fn", sFN},
    {"for", sFOR},
    {"globals", sGLOBALS},
    {"if", sIF},
    {"int", sINT},
    {"locals", sLOCALS},
    {"loop", sLOOP},
    {"main", sMAIN},
    {"match", sMATCH},
    {"module", sMODULE},
    {"otherwise", sOTHERWISE},
    {"print", sPRINT},
    {"proc", sPROC},
    {"ret", sRET},
    {"scan", sSCAN},
    {"start", sSTART},
    {"step", sSTEP},
    {"to", sTO},
    {"true", sTRUE},
    {"until", sUNTIL},
    {"v", sOR},
    {"when", sWHEN},
    {"while", sWHILE}
};

/* funcoes internas do lexer */
static int lex_peek(Lexer *lexer);
static int lex_advance(Lexer *lexer);
static void lex_log_token(Lexer *lexer, const Token *token);
static void lex_skip_ignored(Lexer *lexer);
static void lex_skip_whitespace(Lexer *lexer);
static int lex_skip_comment(Lexer *lexer);
static Token lex_read_identifier_or_keyword(Lexer *lexer);
static Token lex_read_number(Lexer *lexer);
static Token lex_read_string(Lexer *lexer);
static Token lex_read_char(Lexer *lexer);
static Token lex_read_symbol(Lexer *lexer);

void lex_init(Lexer *lexer, FILE *file, FILE *token_log) {
    /* guarda o arquivo e zera a posicao inicial */
    lexer->file = file;
    lexer->token_log = token_log;
    lexer->line = 1;
    lexer->column = 1;
}

void lex_destroy(Lexer *lexer) {
    (void)lexer;
}

Token lex_next(Lexer *lexer) {
    Token token;
    int c;

    /* ignora espacos e comentarios antes de montar o proximo token */
    lex_skip_ignored(lexer);
    c = lex_peek(lexer);

    if (c == EOF) {
        token = token_make(sEOF, "", lexer->line, lexer->column);
        lex_log_token(lexer, &token);
        return token;
    }

    if (isalpha(c) || c == '_') {
        token = lex_read_identifier_or_keyword(lexer);
        lex_log_token(lexer, &token);
        return token;
    }

    if (isdigit(c)) {
        token = lex_read_number(lexer);
        lex_log_token(lexer, &token);
        return token;
    }

    if (c == '"') {
        token = lex_read_string(lexer);
        lex_log_token(lexer, &token);
        return token;
    }

    if (c == '\'') {
        token = lex_read_char(lexer);
        lex_log_token(lexer, &token);
        return token;
    }

    token = lex_read_symbol(lexer);
    lex_log_token(lexer, &token);
    return token;
}

static int lex_peek(Lexer *lexer) {
    int c = fgetc(lexer->file);

    if (c != EOF) {
        ungetc(c, lexer->file);
    }

    return c;
}

static int lex_advance(Lexer *lexer) {
    int c = fgetc(lexer->file);

    /* atualiza linha e coluna para os diagnosticos */
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else if (c != EOF) {
        lexer->column++;
    }

    return c;
}

static void lex_log_token(Lexer *lexer, const Token *token) {
    /* grava o token no arquivo .tk */
    if (lexer->token_log == NULL) {
        return;
    }

    fprintf(lexer->token_log,
            "%d  %s  \"%s\"\n",
            token->line,
            token_category_name(token->category),
            token->lexeme != NULL ? token->lexeme : "");
}

static void lex_skip_ignored(Lexer *lexer) {
    int advanced;

    do {
        advanced = 0;
        lex_skip_whitespace(lexer);
        advanced = lex_skip_comment(lexer);
    } while (advanced);
}

static void lex_skip_whitespace(Lexer *lexer) {
    int c = lex_peek(lexer);

    while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        lex_advance(lexer);
        c = lex_peek(lexer);
    }
}

static int lex_skip_comment(Lexer *lexer) {
    int c;

    /* comentario de linha com @
       comentario de bloco com @{ ... }@ */
    if (lex_peek(lexer) != '@') {
        return 0;
    }

    lex_advance(lexer);
    c = lex_peek(lexer);

    if (c == '{') {
        int prev = 0;

        lex_advance(lexer);
        while ((c = lex_advance(lexer)) != EOF) {
            if (prev == '}' && c == '@') {
                return 1;
            }
            prev = c;
        }
        diag_errorf(lexer->line, lexer->column, "comentario de bloco nao encerrado");
    }

    while ((c = lex_peek(lexer)) != EOF && c != '\n') {
        lex_advance(lexer);
    }

    return 1;
}

static Token lex_read_identifier_or_keyword(Lexer *lexer) {
    char buffer[LEXEME_CAP];
    int length = 0;
    int c = lex_peek(lexer);
    int line = lexer->line;
    int column = lexer->column;
    size_t i;

    while ((isalpha(c) || isdigit(c) || c == '_') && length < LEXEME_CAP - 1) {
        buffer[length++] = (char)lex_advance(lexer);
        c = lex_peek(lexer);
    }
    buffer[length] = '\0';

    /* primeiro tenta palavra reservada
       se nao for, vira identificador */
    for (i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        if (strcmp(buffer, keywords[i].keyword) == 0) {
            return token_make(keywords[i].category, buffer, line, column);
        }
    }

    return token_make(sIDENTIF, buffer, line, column);
}

static Token lex_read_number(Lexer *lexer) {
    char buffer[LEXEME_CAP];
    int length = 0;
    int c = lex_peek(lexer);
    int line = lexer->line;
    int column = lexer->column;

    while (isdigit(c) && length < LEXEME_CAP - 1) {
        buffer[length++] = (char)lex_advance(lexer);
        c = lex_peek(lexer);
    }
    buffer[length] = '\0';

    return token_make(sCTEINT, buffer, line, column);
}

static Token lex_read_string(Lexer *lexer) {
    char buffer[LEXEME_CAP];
    int length = 0;
    int c;
    int line = lexer->line;
    int column = lexer->column;

    lex_advance(lexer);

    while ((c = lex_peek(lexer)) != EOF && c != '"' && c != '\n') {
        if (length >= LEXEME_CAP - 1) {
            diag_errorf(line, column, "string excede tamanho maximo suportado");
        }
        buffer[length++] = (char)lex_advance(lexer);
    }

    if (c != '"') {
        diag_errorf(line, column, "string nao encerrada");
    }

    lex_advance(lexer);
    buffer[length] = '\0';
    return token_make(sSTRING, buffer, line, column);
}

static Token lex_read_char(Lexer *lexer) {
    char buffer[2];
    int c;
    int line = lexer->line;
    int column = lexer->column;

    lex_advance(lexer);
    c = lex_advance(lexer);

    if (c == EOF || c == '\n' || c == '\'') {
        diag_errorf(line, column, "literal char invalido");
    }

    buffer[0] = (char)c;
    buffer[1] = '\0';

    if (lex_peek(lexer) != '\'') {
        diag_errorf(line, column, "literal char deve conter exatamente um caractere");
    }

    lex_advance(lexer);
    return token_make(sCTECHAR, buffer, line, column);
}

static Token lex_read_symbol(Lexer *lexer) {
    int c = lex_advance(lexer);
    int line = lexer->line;
    int column = lexer->column - 1;

    /* aqui ficam operadores e delimitadores */
    switch (c) {
        case ':':
            if (lex_peek(lexer) == '=') {
                lex_advance(lexer);
                return token_make(sATRIB, ":=", line, column);
            }
            return token_make(sCOLON, ":", line, column);
        case '<':
            if (lex_peek(lexer) == '=') {
                lex_advance(lexer);
                return token_make(sMENORIG, "<=", line, column);
            }
            if (lex_peek(lexer) == '>') {
                lex_advance(lexer);
                return token_make(sDIFERENTE, "<>", line, column);
            }
            return token_make(sMENOR, "<", line, column);
        case '>':
            if (lex_peek(lexer) == '=') {
                lex_advance(lexer);
                return token_make(sMAIORIG, ">=", line, column);
            }
            return token_make(sMAIOR, ">", line, column);
        case '=':
            if (lex_peek(lexer) == '>') {
                lex_advance(lexer);
                return token_make(sIMPLIC, "=>", line, column);
            }
            return token_make(sIGUAL, "=", line, column);
        case '.':
            if (lex_peek(lexer) == '.') {
                lex_advance(lexer);
                return token_make(sPTOPTO, "..", line, column);
            }
            break;
        case '+':
            return token_make(sSOMA, "+", line, column);
        case '-':
            return token_make(sSUBRAT, "-", line, column);
        case '*':
            return token_make(sMULT, "*", line, column);
        case '/':
            return token_make(sDIV, "/", line, column);
        case '^':
            return token_make(sAND, "^", line, column);
        case '~':
            return token_make(sNEG, "~", line, column);
        case '(':
            return token_make(sLPAR, "(", line, column);
        case ')':
            return token_make(sRPAR, ")", line, column);
        case '[':
            return token_make(sLBRACK, "[", line, column);
        case ']':
            return token_make(sRBRACK, "]", line, column);
        case ',':
            return token_make(sCOMMA, ",", line, column);
        case ';':
            return token_make(sSEMI, ";", line, column);
    }

    diag_errorf(line, column, "simbolo invalido '%c'", c);
    return token_make(sERROR, "", line, column);
}
