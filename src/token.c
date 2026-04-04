#include <stdlib.h>
#include <string.h>

#include "token.h"

static char *token_strdup(const char *text) {
    size_t size;
    char *copy;

    if (text == NULL) {
        return NULL;
    }

    size = strlen(text) + 1;
    copy = (char *)malloc(size);
    if (copy != NULL) {
        memcpy(copy, text, size);
    }

    return copy;
}

const char *token_category_name(Category category) {
    switch (category) {
        case sIDENTIF: return "sIDENTIF";
        case sCTEINT: return "sCTEINT";
        case sCTECHAR: return "sCTECHAR";
        case sSTRING: return "sSTRING";
        case sMODULE: return "sMODULE";
        case sPROC: return "sPROC";
        case sFN: return "sFN";
        case sMAIN: return "sMAIN";
        case sGLOBALS: return "sGLOBALS";
        case sLOCALS: return "sLOCALS";
        case sSTART: return "sSTART";
        case sEND: return "sEND";
        case sIF: return "sIF";
        case sELSE: return "sELSE";
        case sMATCH: return "sMATCH";
        case sWHEN: return "sWHEN";
        case sOTHERWISE: return "sOTHERWISE";
        case sFOR: return "sFOR";
        case sTO: return "sTO";
        case sSTEP: return "sSTEP";
        case sDO: return "sDO";
        case sLOOP: return "sLOOP";
        case sWHILE: return "sWHILE";
        case sUNTIL: return "sUNTIL";
        case sPRINT: return "sPRINT";
        case sSCAN: return "sSCAN";
        case sRET: return "sRET";
        case sINT: return "sINT";
        case sBOOL: return "sBOOL";
        case sCHAR: return "sCHAR";
        case sTRUE: return "sTRUE";
        case sFALSE: return "sFALSE";
        case sATRIB: return "sATRIB";
        case sSOMA: return "sSOMA";
        case sSUBRAT: return "sSUBRAT";
        case sMULT: return "sMULT";
        case sDIV: return "sDIV";
        case sIGUAL: return "sIGUAL";
        case sDIFERENTE: return "sDIFERENTE";
        case sMAIOR: return "sMAIOR";
        case sMENOR: return "sMENOR";
        case sMAIORIG: return "sMAIORIG";
        case sMENORIG: return "sMENORIG";
        case sAND: return "sAND";
        case sOR: return "sOR";
        case sNEG: return "sNEG";
        case sLPAR: return "sLPAR";
        case sRPAR: return "sRPAR";
        case sLBRACK: return "sLBRACK";
        case sRBRACK: return "sRBRACK";
        case sCOMMA: return "sCOMMA";
        case sSEMI: return "sSEMI";
        case sCOLON: return "sCOLON";
        case sPTOPTO: return "sPTOPTO";
        case sIMPLIC: return "sIMPLIC";
        case sEOF: return "sEOF";
        case sERROR: return "sERROR";
    }

    return "sUNKNOWN";
}

Token token_make(Category category, const char *lexeme, int line, int column) {
    Token token;

    token.category = category;
    token.lexeme = token_strdup(lexeme != NULL ? lexeme : "");
    token.line = line;
    token.column = column;

    return token;
}

void token_free(Token *token) {
    if (token == NULL) {
        return;
    }

    free(token->lexeme);
    token->lexeme = NULL;
}
