#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum Category {
    sIDENTIF,
    sCTEINT,
    sCTECHAR,
    sSTRING,

    sMODULE,
    sPROC,
    sFN,
    sMAIN,
    sGLOBALS,
    sLOCALS,
    sSTART,
    sEND,
    sIF,
    sELSE,
    sMATCH,
    sWHEN,
    sOTHERWISE,
    sFOR,
    sTO,
    sSTEP,
    sDO,
    sLOOP,
    sWHILE,
    sUNTIL,
    sPRINT,
    sSCAN,
    sRET,
    sINT,
    sBOOL,
    sCHAR,
    sTRUE,
    sFALSE,

    sATRIB,
    sSOMA,
    sSUBRAT,
    sMULT,
    sDIV,
    sIGUAL,
    sDIFERENTE,
    sMAIOR,
    sMENOR,
    sMAIORIG,
    sMENORIG,
    sAND,
    sOR,
    sNEG,

    sLPAR,
    sRPAR,
    sLBRACK,
    sRBRACK,
    sCOMMA,
    sSEMI,
    sCOLON,

    sPTOPTO,
    sIMPLIC,

    sEOF,
    sERROR
} Category;

typedef struct {
    char *lexeme;
    Category category;
    int line;
    int column;
} Token;

const char *token_category_name(Category category);
void token_free(Token *token);
Token token_make(Category category, const char *lexeme, int line, int column);

#endif
