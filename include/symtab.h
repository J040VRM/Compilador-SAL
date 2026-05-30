#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>

typedef enum {
    SYM_VAR,
    SYM_PARAM,
    SYM_PROC,
    SYM_FN
} SymbolKind;

typedef enum {
    TYPE_NONE,
    TYPE_INT,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_STRING
} DataType;

#define SYMTAB_MAX_PARAMS 32

typedef struct {
    char lexeme[64];
    SymbolKind kind;
    DataType type;
    int extra;
    int size;
    int scope_id;
    int level;
    int address;
    int param_count;
    DataType param_types[SYMTAB_MAX_PARAMS];
    char label[32];
} Symbol;

void ts_init(void);
void ts_destroy(void);

void ts_enter_named_scope(const char *name);
void ts_enter_block_scope(void);
void ts_exit_scope(void);

int ts_insert(const char *lexeme, SymbolKind kind, DataType type, int extra);
int ts_insert_full(const char *lexeme,
                   SymbolKind kind,
                   DataType type,
                   int extra,
                   int size,
                   int level,
                   int address,
                   const char *label);
int ts_update_global(const char *lexeme, SymbolKind kind, DataType type, int extra);
int ts_update_routine_signature(const char *lexeme,
                                SymbolKind kind,
                                DataType type,
                                int param_count,
                                const DataType *param_types,
                                const char *label);
int ts_update_symbol_address(const char *lexeme, int level, int address);
const Symbol *ts_lookup(const char *lexeme);

const char *ts_current_scope_path(void);
const char *ts_scope_path_from_id(int scope_id);
const char *ts_symbol_kind_name(SymbolKind kind);
const char *ts_data_type_name(DataType type);

void ts_dump(FILE *out);

#endif
