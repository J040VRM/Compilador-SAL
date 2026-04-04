#include <stdio.h>
#include <string.h>

#include "symtab.h"

#define MAX_SYMBOLS 2048
#define MAX_SCOPES 256
#define MAX_PATH 256

typedef struct {
    int id;
    int parent_id;
    char path[MAX_PATH];
    int next_block_index;
} Scope;

static Symbol symbols[MAX_SYMBOLS];
static int symbol_count = 0;

static Scope scopes[MAX_SCOPES];
static int scope_count = 0;
static int current_scope_id = 0;

/* busca um escopo pelo id interno */
static Scope *ts_scope_by_id(int scope_id) {
    int i;

    for (i = 0; i < scope_count; ++i) {
        if (scopes[i].id == scope_id) {
            return &scopes[i];
        }
    }

    return NULL;
}

void ts_init(void) {
    /* escopo zero representa o global */
    symbol_count = 0;
    scope_count = 1;
    current_scope_id = 0;

    scopes[0].id = 0;
    scopes[0].parent_id = -1;
    strcpy(scopes[0].path, "global");
    scopes[0].next_block_index = 1;
}

void ts_destroy(void) {
}

void ts_enter_named_scope(const char *name) {
    Scope *current;
    Scope *scope;

    /* cria um novo escopo e liga com o escopo pai */
    if (scope_count >= MAX_SCOPES) {
        return;
    }

    current = ts_scope_by_id(current_scope_id);
    scope = &scopes[scope_count];
    scope->id = scope_count;
    scope->parent_id = current_scope_id;
    strncpy(scope->path, name, sizeof(scope->path) - 1);
    scope->path[sizeof(scope->path) - 1] = '\0';
    scope->next_block_index = 1;

    if (current != NULL && strcmp(name, "global") != 0 && strchr(name, '.') == NULL &&
        strncmp(name, "proc:", 5) != 0 && strncmp(name, "fn:", 3) != 0) {
        snprintf(scope->path, sizeof(scope->path), "%s.%s", current->path, name);
    }

    current_scope_id = scope->id;
    scope_count++;
}

void ts_enter_block_scope(void) {
    Scope *current;
    char name[64];

    current = ts_scope_by_id(current_scope_id);
    if (current == NULL) {
        return;
    }

    /* blocos internos recebem nomes automaticos */
    snprintf(name, sizeof(name), "%s.block#%d", current->path, current->next_block_index);
    current->next_block_index++;
    ts_enter_named_scope(name);
}

void ts_exit_scope(void) {
    Scope *scope = ts_scope_by_id(current_scope_id);

    if (scope != NULL && scope->parent_id >= 0) {
        current_scope_id = scope->parent_id;
    }
}

int ts_insert(const char *lexeme, SymbolKind kind, DataType type, int extra) {
    int i;

    if (symbol_count >= MAX_SYMBOLS) {
        return 0;
    }

    /* nao deixa repetir nome no mesmo escopo */
    for (i = symbol_count - 1; i >= 0; --i) {
        if (symbols[i].scope_id != current_scope_id) {
            continue;
        }

        if (strcmp(symbols[i].lexeme, lexeme) == 0) {
            return 0;
        }
    }

    strncpy(symbols[symbol_count].lexeme, lexeme, sizeof(symbols[symbol_count].lexeme) - 1);
    symbols[symbol_count].lexeme[sizeof(symbols[symbol_count].lexeme) - 1] = '\0';
    symbols[symbol_count].kind = kind;
    symbols[symbol_count].type = type;
    symbols[symbol_count].extra = extra;
    symbols[symbol_count].scope_id = current_scope_id;
    symbol_count++;

    return 1;
}

int ts_update_global(const char *lexeme, SymbolKind kind, DataType type, int extra) {
    int i;

    for (i = symbol_count - 1; i >= 0; --i) {
        if (symbols[i].scope_id == 0 &&
            symbols[i].kind == kind &&
            strcmp(symbols[i].lexeme, lexeme) == 0) {
            symbols[i].type = type;
            symbols[i].extra = extra;
            return 1;
        }
    }

    return 0;
}

const Symbol *ts_lookup(const char *lexeme) {
    int scope_id = current_scope_id;

    /* procura do escopo atual ate chegar no global */
    while (scope_id >= 0) {
        int i;

        for (i = symbol_count - 1; i >= 0; --i) {
            if (symbols[i].scope_id == scope_id && strcmp(symbols[i].lexeme, lexeme) == 0) {
                return &symbols[i];
            }
        }

        {
            Scope *scope = ts_scope_by_id(scope_id);
            scope_id = scope != NULL ? scope->parent_id : -1;
        }
    }

    return NULL;
}

const char *ts_current_scope_path(void) {
    Scope *scope = ts_scope_by_id(current_scope_id);
    return scope != NULL ? scope->path : "global";
}

const char *ts_scope_path_from_id(int scope_id) {
    Scope *scope = ts_scope_by_id(scope_id);
    return scope != NULL ? scope->path : "unknown";
}

const char *ts_symbol_kind_name(SymbolKind kind) {
    switch (kind) {
        case SYM_VAR: return "var";
        case SYM_PARAM: return "param";
        case SYM_PROC: return "proc";
        case SYM_FN: return "fn";
    }

    return "unknown";
}

const char *ts_data_type_name(DataType type) {
    switch (type) {
        case TYPE_NONE: return "-";
        case TYPE_INT: return "int";
        case TYPE_BOOL: return "bool";
        case TYPE_CHAR: return "char";
    }

    return "-";
}

void ts_dump(FILE *out) {
    int scope_index;

    /* imprime agrupando por escopo para facilitar estudo */
    for (scope_index = 0; scope_index < scope_count; ++scope_index) {
        int symbol_index;
        int scope_id = scopes[scope_index].id;

        for (symbol_index = 0; symbol_index < symbol_count; ++symbol_index) {
            if (symbols[symbol_index].scope_id != scope_id) {
                continue;
            }

            fprintf(out,
                    "SCOPE=%s  id=\"%s\"  cat=%s  tipo=%s  extra=%d\n",
                    ts_scope_path_from_id(symbols[symbol_index].scope_id),
                    symbols[symbol_index].lexeme,
                    ts_symbol_kind_name(symbols[symbol_index].kind),
                    ts_data_type_name(symbols[symbol_index].type),
                    symbols[symbol_index].extra);
        }
    }
}
