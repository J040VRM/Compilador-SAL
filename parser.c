#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "token.h"

static Token current_token;
static Lexer *parser_lexer;

static void next_token();
static void match(enum Category expected);

static void program();
static void block();

static void stmt_list();
static void stmt();
static void expr();
static void term();
static void factor();

void parse_program(Lexer *lexer){
    parser_lexer = lexer;
    next_token();
    program();
}

static void next_token(){
    current_token = lex_next(parser_lexer);
}

static void match (enum Category expected){
    if ( current_token.category == expected){
        next_token();
    }else{
        printf("Erro sintatico na linha %d\n", current_token.line);
        printf("Esperado: %d | Encontrado: %d\n",
        expected, current_token.category);
        exit(1);  
    }
}

static void stmt_list(){
    while (current_token.category == sIDENTIF || current_token.category == sIF)
    {
        stmt();
    }
    
}

static void stmt() {
    if (current_token.category == sIDENTIF) {
        match(sIDENTIF);
        match(sATRIB);
        expr(); 
        match(sSEMI);
    }
    else if (current_token.category == sIF) {
        match(sIF);
        expr();
        match(sTHEN);
        stmt_list();
        match(sEND);
    }
    else {
        printf("Erro: comando inválido na linha %d\n", current_token.line);
        exit(1);
    }
}

static void block(){
    match(sSTART);

    stmt_list();

    match(sEND);
}

static void program() {
    match(sMODULE);
    match(sIDENTIF);
    match(sSEMI);

    block();
}

static void factor() {
    if (current_token.category == sIDENTIF) {
        match(sIDENTIF);
    } else if (current_token.category == sCTEINT) {
        match(sCTEINT);
    } else if (current_token.category == sLPAR) {
        match(sLPAR);
        expr();
        match(sRPAR);
    } else {
        printf("Erro: fator inválido na linha %d\n", current_token.line);
        exit(1);
    }
}

static void term() {
    factor();
    while (current_token.category == sMULT || current_token.category == sDIV) {
        if (current_token.category == sMULT) {
            match(sMULT);
        } else {
            match(sDIV);
        }
        factor();
    }
}

static void expr() {
    term();

    while (current_token.category == sSOMA || current_token.category == sSUBRAT) {
        if (current_token.category == sSOMA) {
            match(sSOMA);
        } else {
            match(sSUBRAT);
        }
        term();
    }

    if (current_token.category == sMAIOR ||
        current_token.category == sMENOR ||
        current_token.category == sIGUAL ||
        current_token.category == sMAIORIG ||
        current_token.category == sMENORIG ||
        current_token.category == sDIFERENTE) {

        enum Category op = current_token.category;
        match(op);
        term();
    }
}