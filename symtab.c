#include <stdio.h>
#include <string.h>
#include "symtab.h"

#define MAX_SYMBOLS 1000

static Symbol table[MAX_SYMBOLS];
static int top = 0;
static int current_scope = 0;

void ts_init(){
    top = 0;
    current_scope = 0;
}

void ts_enter_scope(){
    current_scope++;
}

void ts_exit_scope(){
    while(top>0 && table[top-1].scope == current_scope){
        top --;
    }
    current_scope --;
}

int ts_insert(const char *lexeme, int category, int type) {
    for (int i = top - 1; i >= 0; i--) {
        if (table[i].scope != current_scope) break;

        if (strcmp(table[i].lexeme, lexeme) == 0) {
            return 0; // já existe
        }
    }

    strcpy(table[top].lexeme, lexeme);
    table[top].category = category;
    table[top].type = type;
    table[top].scope = current_scope;

    top++;
    return 1;
}

Symbol* ts_lookup(const char *lexeme) {
    for (int i = top - 1; i >= 0; i--) {
        if (strcmp(table[i].lexeme, lexeme) == 0) {
            return &table[i];
        }
    }
    return NULL;
}