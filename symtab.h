#ifndef SYMTAB_H
#define SYMTAB_H

typedef struct Symbol {
    char lexeme[64];
    int category;
    int type;
    int scope;
} Symbol;

void ts_init();
void ts_enter_scope();
void ts_exit_scope();

int ts_insert(const char *lexeme, int category, int type);
Symbol* ts_lookup(const char *lexeme);

#endif