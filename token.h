#ifndef TOKEN_H
#define TOKEN_H

enum Category {
    /* Identificadores e literais */
    sIDENTIF,
    sCTEINT,
    sCTECHAR,
    sSTRING,

    /* Palavras reservadas */
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
    sTHEN,
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

    /* Operadores */
    sATRIB,       /* := */
    sSOMA,        /* + */
    sSUBRAT,      /* - */
    sMULT,        /* * */
    sDIV,         /* / */
    sIGUAL,       /* = */
    sDIFERENTE,   /* <> */
    sMAIOR,       /* > */
    sMENOR,       /* < */
    sMAIORIG,     /* >= */
    sMENORIG,     /* <= */
    sAND,         /* ^ */
    sOR,          /* v */
    sNEG,         /* ~ */

    /* Delimitadores */
    sLPAR,        /* ( */
    sRPAR,        /* ) */
    sLBRACK,      /* [ */
    sRBRACK,      /* ] */
    sCOMMA,       /* , */
    sSEMI,        /* ; */
    sCOLON,       /* : */

    /* Especiais */
    sPTOPTO,      /* .. */
    sIMPLIC,      /* => */

    /* Controle */
    sEOF,
    sERROR
};

typedef struct {
    char *lexeme;
    enum Category category;
    int line;
    int column;
} Token;

#endif