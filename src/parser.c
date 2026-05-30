#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag.h"
#include "gerador.h"
#include "parser.h"
#include "symtab.h"

static Lexer *parser_lexer = NULL;
static Token current_token;

static int global_next_addr = 0;
static int local_next_addr = 0;
static int current_local_alloc = 0;
static int current_mepa_level = 0;
static int current_param_count = 0;
static int current_temp_addr = -1;
static int current_temp2_addr = -1;
static int current_return_addr = -1;
static int current_temp_pool_base = -1;
static int current_temp_pool_next = 0;
static int current_decl_is_global = 0;
static int program_needs_temp = 0;
static int last_reference_indirect = 0;
static SymbolKind current_routine_kind = SYM_PROC;
static DataType current_return_type = TYPE_NONE;
static int current_function_has_return = 0;
static char *main_label = NULL;

typedef struct {
    int level;
    int address;
    int size;
} VectorInit;

#define MAX_VECTOR_INITS 256
#define TEMP_POOL_SIZE 16
static VectorInit global_vector_inits[MAX_VECTOR_INITS];
static int global_vector_init_count = 0;
static VectorInit local_vector_inits[MAX_VECTOR_INITS];
static int local_vector_init_count = 0;

static void next_token(void);
static void consume(Category expected);
static int accept(Category category);
static Token peek_token(void);
static void trace_enter(const char *name);
static void trace_exit(const char *name);
static int scan_program_needs_temp(Lexer *lexer);

static void parse_module(void);
static void parse_globals(void);
static void parse_declaration_list(void);
static void parse_declaration(void);
static DataType parse_type(void);
static void parse_subroutine_list(void);
static void parse_subroutine(void);
static void parse_main(void);
static int parse_params_and_count(DataType *param_types, int max_params);
static int parse_optional_locals(const char *scope_name);
static void parse_block(int create_scope);
static void parse_statement_list(Category terminator_a, Category terminator_b);
static void parse_statement(void);
static void parse_embedded_command(void);
static void parse_assignment_or_call(int require_semicolon);
static void parse_assignment_tail(const char *identifier, int line, int column, int require_semicolon);
static DataType parse_call_tail(const char *identifier, int line, int column, int require_semicolon, int as_expression);
static void parse_scan(int require_semicolon);
static void parse_print(int require_semicolon);
static void parse_print_item(void);
static void parse_if(int require_semicolon);
static void parse_match(int require_semicolon);
static void parse_for(int require_semicolon);
static void parse_loop(int require_semicolon);
static void parse_return(int require_semicolon);
static const Symbol *parse_variable_reference(int require_declared, int emit_load);
static int parse_argument_list(const Symbol *routine);
static void parse_when_condition(const char *body_label);
static void parse_when_item(const char *body_label);
static int parse_range_int(void);

static DataType parse_expr(void);
static DataType parse_or_expr(void);
static DataType parse_and_expr(void);
static DataType parse_rel_expr(void);
static DataType parse_add_expr(void);
static DataType parse_mul_expr(void);
static DataType parse_unary_expr(void);
static DataType parse_primary_expr(void);

static DataType token_to_type(Category category);
static const Symbol *require_symbol(const char *name, int line, int column);
static void require_assignable_symbol(const Symbol *symbol, const char *name, int line, int column);
static void require_type(DataType found, DataType expected, int line, int column, const char *context);
static int same_type(DataType a, DataType b);
static char *copy_lexeme(const Token *token, char *buffer, size_t size);
static void emit0(const char *mnemonic);
static void emit1(const char *mnemonic, const char *param);
static void emit1_int(const char *mnemonic, int value);
static void emit2_int(const char *mnemonic, int a, int b);
static void emit_label(const char *label);
static void emit_string_literal(const char *text);
static int symbol_storage_size(int extra);
static void add_vector_init(int level, int address, int size);
static void emit_vector_inits(VectorInit *items, int count);
static void emit_vector_address_to(const Symbol *symbol, int line, int column, int temp_addr);
static void reserve_temp_if_needed(void);
static int alloc_temp_slot(int line, int column);
static void free_temp_slots(int count);

void parse_program(Lexer *lexer) {
    parser_lexer = lexer;
    global_next_addr = 0;
    local_next_addr = 0;
    current_local_alloc = 0;
    current_mepa_level = 0;
    current_param_count = 0;
    current_temp_addr = -1;
    current_temp2_addr = -1;
    current_temp_pool_base = -1;
    current_temp_pool_next = 0;
    current_return_addr = -1;
    current_decl_is_global = 0;
    program_needs_temp = scan_program_needs_temp(lexer);
    last_reference_indirect = 0;
    current_return_type = TYPE_NONE;
    current_function_has_return = 0;
    global_vector_init_count = 0;
    local_vector_init_count = 0;
    main_label = novo_rotulo();

    next_token();
    parse_module();
    token_free(&current_token);
}

static void next_token(void) {
    token_free(&current_token);
    current_token = lex_next(parser_lexer);
}

static void consume(Category expected) {
    if (current_token.category != expected) {
        diag_syntax_expected(expected, &current_token);
    }
    if (expected == sEOF) {
        return;
    }
    next_token();
}

static int accept(Category category) {
    if (current_token.category == category) {
        next_token();
        return 1;
    }
    return 0;
}

static Token peek_token(void) {
    Lexer temp = *parser_lexer;
    fpos_t pos;
    int saved_line = parser_lexer->line;
    int saved_column = parser_lexer->column;
    Token token;

    fgetpos(parser_lexer->file, &pos);
    temp.token_log = NULL;
    token = lex_next(&temp);
    fsetpos(parser_lexer->file, &pos);
    parser_lexer->line = saved_line;
    parser_lexer->column = saved_column;
    return token;
}

static void trace_enter(const char *name) {
    diag_info("enter %s token=%s \"%s\" line=%d",
              name,
              token_category_name(current_token.category),
              current_token.lexeme != NULL ? current_token.lexeme : "",
              current_token.line);
}

static void trace_exit(const char *name) {
    diag_info("exit %s token=%s \"%s\" line=%d",
              name,
              token_category_name(current_token.category),
              current_token.lexeme != NULL ? current_token.lexeme : "",
              current_token.line);
}

static int scan_program_needs_temp(Lexer *lexer) {
    Lexer temp = *lexer;
    Token token;
    fpos_t pos;
    int saved_line = lexer->line;
    int saved_column = lexer->column;
    int needs_temp = 0;

    fgetpos(lexer->file, &pos);
    temp.token_log = NULL;

    do {
        token = lex_next(&temp);
        if (token.category == sMATCH || token.category == sLBRACK ||
            token.category == sFOR || token.category == sSTEP) {
            needs_temp = 1;
        }
        if (token.category == sEOF) {
            token_free(&token);
            break;
        }
        token_free(&token);
    } while (!needs_temp);

    fsetpos(lexer->file, &pos);
    lexer->line = saved_line;
    lexer->column = saved_column;
    return needs_temp;
}

static void parse_module(void) {
    trace_enter("program");

    emit0("INPP");

    consume(sMODULE);
    consume(sIDENTIF);
    consume(sSEMI);

    if (accept(sGLOBALS)) {
        parse_globals();
    }

    if (global_next_addr > 0) {
        emit1_int("AMEM", global_next_addr);
        emit_vector_inits(global_vector_inits, global_vector_init_count);
    }
    emit1("DSVS", main_label);

    parse_subroutine_list();
    parse_main();
    consume(sEOF);

    trace_exit("program");
}

static void parse_globals(void) {
    trace_enter("globals");
    current_decl_is_global = 1;
    parse_declaration_list();
    current_decl_is_global = 0;
    trace_exit("globals");
}

static void parse_declaration_list(void) {
    while (current_token.category == sIDENTIF) {
        parse_declaration();
    }
}

static void parse_declaration(void) {
    char names[64][64];
    int extras[64];
    int addresses[64];
    int count = 0;
    DataType type;

    trace_enter("decl");

    do {
        if (count >= 64) {
            diag_errorf(current_token.line, current_token.column, "declaracao excede limite interno");
        }

        copy_lexeme(&current_token, names[count], sizeof(names[count]));
        consume(sIDENTIF);

        extras[count] = 0;
        if (accept(sLBRACK)) {
            if (current_token.category != sCTEINT) {
                diag_syntax_expected(sCTEINT, &current_token);
            }
            extras[count] = (int)strtol(current_token.lexeme, NULL, 10);
            if (extras[count] <= 0) {
                diag_errorf(current_token.line, current_token.column, "vetor deve ter tamanho positivo");
            }
            consume(sCTEINT);
            consume(sRBRACK);
        }

        if (current_decl_is_global) {
            addresses[count] = global_next_addr;
            global_next_addr += symbol_storage_size(extras[count]);
            if (extras[count] > 0) {
                add_vector_init(0, addresses[count], extras[count]);
            }
        } else if (current_mepa_level == 0) {
            addresses[count] = global_next_addr + local_next_addr;
            local_next_addr += symbol_storage_size(extras[count]);
            current_local_alloc += symbol_storage_size(extras[count]);
            if (extras[count] > 0) {
                add_vector_init(0, addresses[count], extras[count]);
            }
        } else {
            addresses[count] = local_next_addr;
            local_next_addr += symbol_storage_size(extras[count]);
            current_local_alloc += symbol_storage_size(extras[count]);
            if (extras[count] > 0) {
                add_vector_init(current_mepa_level, addresses[count], extras[count]);
            }
        }

        count++;
    } while (accept(sCOMMA));

    consume(sCOLON);
    type = parse_type();
    consume(sSEMI);

    {
        int i;
        for (i = 0; i < count; ++i) {
            int size = symbol_storage_size(extras[i]);
            if (!ts_insert_full(names[i],
                                SYM_VAR,
                                type,
                                extras[i],
                                size,
                                current_mepa_level,
                                addresses[i],
                                NULL)) {
                diag_errorf(current_token.line,
                            current_token.column,
                            "identificador \"%s\" ja declarado no escopo %s",
                            names[i],
                            ts_current_scope_path());
            }
        }
    }

    trace_exit("decl");
}

static DataType parse_type(void) {
    DataType type = token_to_type(current_token.category);

    if (type == TYPE_NONE) {
        diag_errorf(current_token.line, current_token.column, "tipo invalido \"%s\"", current_token.lexeme);
    }

    next_token();
    return type;
}

static void parse_subroutine_list(void) {
    while (current_token.category == sFN || current_token.category == sPROC) {
        if (current_token.category == sPROC) {
            Token lookahead = peek_token();
            int is_main = lookahead.category == sMAIN;
            token_free(&lookahead);
            if (is_main) {
                break;
            }
        }
        parse_subroutine();
    }
}

static void parse_subroutine(void) {
    Category routine_token = current_token.category;
    SymbolKind kind = routine_token == sFN ? SYM_FN : SYM_PROC;
    DataType return_type = TYPE_NONE;
    DataType param_types[SYMTAB_MAX_PARAMS];
    char name[64];
    char scope_name[96];
    char *label = novo_rotulo();
    int param_count;
    int has_locals;
    int saved_level = current_mepa_level;
    int saved_param_count = current_param_count;
    int saved_temp_addr = current_temp_addr;
    int saved_temp2_addr = current_temp2_addr;
    int saved_temp_pool_base = current_temp_pool_base;
    int saved_temp_pool_next = current_temp_pool_next;
    int saved_return_addr = current_return_addr;
    int saved_local_alloc = current_local_alloc;
    int saved_local_next = local_next_addr;
    SymbolKind saved_kind = current_routine_kind;
    DataType saved_return_type = current_return_type;
    int saved_has_return = current_function_has_return;

    trace_enter("subroutine");

    next_token();
    if (current_token.category == sMAIN) {
        diag_errorf(current_token.line, current_token.column, "proc main deve ser declarado por ultimo");
    }

    copy_lexeme(&current_token, name, sizeof(name));
    if (!ts_insert_full(name, kind, TYPE_NONE, 0, 1, 0, -1, label)) {
        diag_errorf(current_token.line, current_token.column, "sub-rotina \"%s\" ja declarada", name);
    }
    consume(sIDENTIF);
    consume(sLPAR);

    snprintf(scope_name, sizeof(scope_name), "%s:%s", kind == SYM_FN ? "fn" : "proc", name);
    ts_enter_named_scope(scope_name);
    current_mepa_level = 1;
    local_next_addr = 0;
    current_param_count = 0;
    current_temp_addr = -1;
    current_temp2_addr = -1;
    current_temp_pool_base = -1;
    current_temp_pool_next = 0;
    current_return_addr = -1;
    current_local_alloc = 0;
    local_vector_init_count = 0;
    current_routine_kind = kind;
    current_return_type = TYPE_NONE;
    current_function_has_return = 0;

    param_count = parse_params_and_count(param_types, SYMTAB_MAX_PARAMS);
    current_param_count = param_count;
    consume(sRPAR);

    if (routine_token == sFN) {
        consume(sCOLON);
        return_type = parse_type();
    }

    current_return_type = return_type;
    if (kind == SYM_FN) {
        current_return_addr = -(param_count + 3);
    }
    if (!ts_update_routine_signature(name, kind, return_type, param_count, param_types, label)) {
        diag_errorf(current_token.line, current_token.column, "falha ao atualizar assinatura de \"%s\"", name);
    }

    has_locals = parse_optional_locals(scope_name);
    reserve_temp_if_needed();

    emit_label(label);
    emit1_int("ENPR", current_mepa_level);
    if (current_local_alloc > 0) {
        emit1_int("AMEM", current_local_alloc);
        emit_vector_inits(local_vector_inits, local_vector_init_count);
    }

    parse_block(1);

    if (kind == SYM_FN && !current_function_has_return) {
        diag_errorf(current_token.line, current_token.column, "funcao \"%s\" deve retornar valor", name);
    }
    if (kind == SYM_PROC) {
        if (current_local_alloc > 0) {
            emit1_int("DMEM", current_local_alloc);
        }
        emit2_int("RTPR", current_mepa_level, param_count);
    }

    if (has_locals) {
        ts_exit_scope();
    }
    ts_exit_scope();

    current_mepa_level = saved_level;
    current_param_count = saved_param_count;
    current_temp_addr = saved_temp_addr;
    current_temp2_addr = saved_temp2_addr;
    current_temp_pool_base = saved_temp_pool_base;
    current_temp_pool_next = saved_temp_pool_next;
    current_return_addr = saved_return_addr;
    current_local_alloc = saved_local_alloc;
    local_next_addr = saved_local_next;
    current_routine_kind = saved_kind;
    current_return_type = saved_return_type;
    current_function_has_return = saved_has_return;

    trace_exit("subroutine");
}

static void parse_main(void) {
    int has_locals;
    int saved_level = current_mepa_level;
    int saved_param_count = current_param_count;
    int saved_temp_addr = current_temp_addr;
    int saved_temp2_addr = current_temp2_addr;
    int saved_temp_pool_base = current_temp_pool_base;
    int saved_temp_pool_next = current_temp_pool_next;
    int saved_local_alloc = current_local_alloc;
    int saved_local_next = local_next_addr;
    SymbolKind saved_kind = current_routine_kind;
    DataType saved_return_type = current_return_type;

    trace_enter("main");

    consume(sPROC);
    consume(sMAIN);
    consume(sLPAR);
    consume(sRPAR);

    if (!ts_insert_full("main", SYM_PROC, TYPE_NONE, 0, 1, 0, -1, main_label)) {
        diag_errorf(current_token.line, current_token.column, "sub-rotina \"main\" ja declarada");
    }

    ts_enter_named_scope("proc:main");
    current_mepa_level = 0;
    current_param_count = 0;
    current_temp_addr = -1;
    current_temp2_addr = -1;
    current_temp_pool_base = -1;
    current_temp_pool_next = 0;
    local_next_addr = 0;
    current_local_alloc = 0;
    local_vector_init_count = 0;
    current_routine_kind = SYM_PROC;
    current_return_type = TYPE_NONE;
    has_locals = parse_optional_locals("proc:main");
    reserve_temp_if_needed();

    emit_label(main_label);
    if (current_local_alloc > 0) {
        emit1_int("AMEM", current_local_alloc);
        emit_vector_inits(local_vector_inits, local_vector_init_count);
    }
    parse_block(1);
    if (current_local_alloc > 0) {
        emit1_int("DMEM", current_local_alloc);
    }
    if (global_next_addr > 0) {
        emit1_int("DMEM", global_next_addr);
    }
    emit0("PARA");
    emit0("FIM");

    if (has_locals) {
        ts_exit_scope();
    }
    ts_exit_scope();

    current_mepa_level = saved_level;
    current_param_count = saved_param_count;
    current_temp_addr = saved_temp_addr;
    current_temp2_addr = saved_temp2_addr;
    current_temp_pool_base = saved_temp_pool_base;
    current_temp_pool_next = saved_temp_pool_next;
    current_local_alloc = saved_local_alloc;
    local_next_addr = saved_local_next;
    current_routine_kind = saved_kind;
    current_return_type = saved_return_type;

    trace_exit("main");
}

static int parse_params_and_count(DataType *param_types, int max_params) {
    int count = 0;
    char names[SYMTAB_MAX_PARAMS][64];

    trace_enter("params");

    if (current_token.category == sIDENTIF) {
        while (1) {
            DataType type;

            if (count >= max_params) {
                diag_errorf(current_token.line, current_token.column, "quantidade maxima de parametros excedida");
            }

            copy_lexeme(&current_token, names[count], sizeof(names[count]));
            consume(sIDENTIF);
            consume(sCOLON);
            type = parse_type();

            param_types[count] = type;
            count++;

            if (!accept(sCOMMA)) {
                break;
            }
        }
    }

    {
        int i;
        for (i = 0; i < count; ++i) {
            int address = i - count - 2;
            if (!ts_insert_full(names[i], SYM_PARAM, param_types[i], 0, 1, current_mepa_level, address, NULL)) {
                diag_errorf(current_token.line, current_token.column, "parametro \"%s\" ja declarado", names[i]);
            }
        }
    }

    trace_exit("params");
    return count;
}

static int parse_optional_locals(const char *scope_name) {
    char locals_scope[128];

    if (!accept(sLOCALS)) {
        return 0;
    }

    snprintf(locals_scope, sizeof(locals_scope), "%s.locals", scope_name);
    ts_enter_named_scope(locals_scope);
    parse_declaration_list();
    return 1;
}

static void parse_block(int create_scope) {
    trace_enter("block");
    consume(sSTART);

    if (create_scope) {
        ts_enter_block_scope();
    }

    parse_statement_list(sEND, sEOF);
    consume(sEND);

    if (create_scope) {
        ts_exit_scope();
    }

    trace_exit("block");
}

static void parse_statement_list(Category terminator_a, Category terminator_b) {
    while (current_token.category != terminator_a &&
           current_token.category != terminator_b &&
           current_token.category != sUNTIL &&
           current_token.category != sOTHERWISE &&
           current_token.category != sWHEN) {
        parse_statement();
    }
}

static void parse_statement(void) {
    trace_enter("stmt");

    switch (current_token.category) {
        case sIDENTIF:
            parse_assignment_or_call(1);
            break;
        case sPRINT:
            parse_print(1);
            break;
        case sSCAN:
            parse_scan(1);
            break;
        case sIF:
            parse_if(1);
            break;
        case sMATCH:
            parse_match(1);
            break;
        case sFOR:
            parse_for(1);
            break;
        case sLOOP:
            parse_loop(1);
            break;
        case sRET:
            parse_return(1);
            break;
        case sSTART:
            parse_block(1);
            break;
        default:
            diag_errorf(current_token.line,
                        current_token.column,
                        "comando invalido: %s \"%s\"",
                        token_category_name(current_token.category),
                        current_token.lexeme);
    }

    trace_exit("stmt");
}

static void parse_embedded_command(void) {
    switch (current_token.category) {
        case sIDENTIF:
            parse_assignment_or_call(0);
            break;
        case sPRINT:
            parse_print(0);
            break;
        case sSCAN:
            parse_scan(0);
            break;
        case sIF:
            parse_if(0);
            break;
        case sMATCH:
            parse_match(0);
            break;
        case sFOR:
            parse_for(0);
            break;
        case sLOOP:
            parse_loop(0);
            break;
        case sRET:
            parse_return(0);
            break;
        case sSTART:
            parse_block(1);
            break;
        default:
            diag_errorf(current_token.line, current_token.column, "comando embutido invalido");
    }
}

static void parse_assignment_or_call(int require_semicolon) {
    char name[64];
    int line = current_token.line;
    int column = current_token.column;

    copy_lexeme(&current_token, name, sizeof(name));
    consume(sIDENTIF);

    if (current_token.category == sLPAR) {
        DataType type = parse_call_tail(name, line, column, require_semicolon, 0);
        if (type != TYPE_NONE) {
            emit1_int("DMEM", 1);
        }
    } else {
        parse_assignment_tail(name, line, column, require_semicolon);
    }
}

static void parse_assignment_tail(const char *identifier, int line, int column, int require_semicolon) {
    const Symbol *symbol = require_symbol(identifier, line, column);
    DataType expr_type;
    int indexed = 0;
    int lhs_temp = -1;

    require_assignable_symbol(symbol, identifier, line, column);

    if (accept(sLBRACK)) {
        if (symbol->extra <= 0) {
            diag_errorf(line, column, "\"%s\" nao e vetor", identifier);
        }
        lhs_temp = alloc_temp_slot(line, column);
        emit_vector_address_to(symbol, line, column, lhs_temp);
        indexed = 1;
    }

    if (symbol->extra > 0 && !indexed) {
        diag_errorf(line, column, "vetor \"%s\" precisa ser indexado", identifier);
    }

    consume(sATRIB);
    expr_type = parse_expr();
    require_type(expr_type, symbol->type, line, column, "atribuicao");
    if (indexed) {
        emit2_int("ARMI", current_mepa_level, lhs_temp);
        free_temp_slots(1);
    } else {
        emit2_int("ARMZ", symbol->level, symbol->address);
    }

    if (require_semicolon) {
        consume(sSEMI);
    }
}

static DataType parse_call_tail(const char *identifier, int line, int column, int require_semicolon, int as_expression) {
    const Symbol *symbol = ts_lookup(identifier);

    if (symbol == NULL || (symbol->kind != SYM_PROC && symbol->kind != SYM_FN)) {
        diag_errorf(line, column, "sub-rotina \"%s\" nao declarada", identifier);
    }
    if (as_expression && symbol->kind != SYM_FN) {
        diag_errorf(line, column, "procedimento \"%s\" nao pode ser usado em expressao", identifier);
    }

    if (symbol->kind == SYM_FN) {
        emit1_int("AMEM", 1);
    }

    consume(sLPAR);
    if (current_token.category != sRPAR) {
        parse_argument_list(symbol);
    } else if (symbol->param_count != 0) {
        diag_errorf(line,
                    column,
                    "sub-rotina \"%s\" espera %d argumento(s), recebeu 0",
                    identifier,
                    symbol->param_count);
    }
    consume(sRPAR);

    if (symbol->label[0] != '\0') {
        char level[16];
        snprintf(level, sizeof(level), "%d", current_mepa_level);
        gera_instr_mepa("", "CHPR", (char *)symbol->label, level);
    }

    if (require_semicolon) {
        consume(sSEMI);
    }

    return symbol->type;
}

static void parse_scan(int require_semicolon) {
    const Symbol *symbol;

    consume(sSCAN);
    consume(sLPAR);
    symbol = parse_variable_reference(1, 0);
    require_assignable_symbol(symbol, symbol->lexeme, current_token.line, current_token.column);
    consume(sRPAR);

    emit0("LEIT");
    if (last_reference_indirect) {
        emit2_int("ARMI", current_mepa_level, current_temp_addr);
    } else {
        emit2_int("ARMZ", symbol->level, symbol->address);
    }

    if (require_semicolon) {
        consume(sSEMI);
    }
}

static void parse_print(int require_semicolon) {
    consume(sPRINT);
    consume(sLPAR);
    parse_print_item();
    while (accept(sCOMMA)) {
        parse_print_item();
    }
    consume(sRPAR);
    if (require_semicolon) {
        consume(sSEMI);
    }
}

static void parse_print_item(void) {
    if (current_token.category == sSTRING) {
        emit_string_literal(current_token.lexeme != NULL ? current_token.lexeme : "");
        consume(sSTRING);
        return;
    }

    parse_expr();
    emit0("IMPR");
}

static void parse_if(int require_semicolon) {
    char *else_label = novo_rotulo();
    char *end_label = novo_rotulo();
    DataType cond_type;

    consume(sIF);
    consume(sLPAR);
    cond_type = parse_expr();
    require_type(cond_type, TYPE_BOOL, current_token.line, current_token.column, "condicao do if");
    consume(sRPAR);
    emit1("DSVF", else_label);

    parse_embedded_command();
    emit1("DSVS", end_label);
    emit_label(else_label);

    if (accept(sELSE)) {
        parse_embedded_command();
    }

    emit_label(end_label);
    if (require_semicolon && current_token.category == sSEMI) {
        consume(sSEMI);
    }
}

static void parse_match(int require_semicolon) {
    int when_count = 0;
    DataType match_type;
    char *end_label = novo_rotulo();

    consume(sMATCH);
    consume(sLPAR);
    match_type = parse_expr();
    require_type(match_type, TYPE_INT, current_token.line, current_token.column, "expressao do match");
    emit2_int("ARMZ", current_mepa_level, current_temp_addr);
    consume(sRPAR);

    while (accept(sWHEN)) {
        char *body_label = novo_rotulo();
        char *next_label = novo_rotulo();
        when_count++;
        parse_when_condition(body_label);
        emit1("DSVS", next_label);
        emit_label(body_label);
        consume(sIMPLIC);
        parse_embedded_command();
        consume(sSEMI);
        emit1("DSVS", end_label);
        emit_label(next_label);
    }

    if (when_count == 0) {
        diag_errorf(current_token.line,
                    current_token.column,
                    "match deve conter ao menos uma clausula when");
    }

    if (accept(sOTHERWISE)) {
        consume(sIMPLIC);
        parse_embedded_command();
        consume(sSEMI);
    }

    emit_label(end_label);
    consume(sEND);
    if (require_semicolon && current_token.category == sSEMI) {
        consume(sSEMI);
    }
}

static void parse_for(int require_semicolon) {
    char control_name[64];
    int line;
    int column;
    const Symbol *control;
    char *start_label = novo_rotulo();
    char *end_label = novo_rotulo();
    char *negative_step_label = novo_rotulo();
    char *after_condition_label = novo_rotulo();
    DataType expr_type;
    int step_value = 1;
    int limit_temp = alloc_temp_slot(current_token.line, current_token.column);
    int step_temp = alloc_temp_slot(current_token.line, current_token.column);

    consume(sFOR);
    copy_lexeme(&current_token, control_name, sizeof(control_name));
    line = current_token.line;
    column = current_token.column;
    consume(sIDENTIF);
    control = require_symbol(control_name, line, column);
    require_assignable_symbol(control, control_name, line, column);
    require_type(control->type, TYPE_INT, line, column, "variavel de controle do for");

    if (accept(sLBRACK)) {
        DataType index_type = parse_expr();
        require_type(index_type, TYPE_INT, line, column, "indice de vetor");
        emit1_int("DMEM", 1);
        consume(sRBRACK);
    }

    consume(sATRIB);
    expr_type = parse_expr();
    require_type(expr_type, TYPE_INT, line, column, "valor inicial do for");
    emit2_int("ARMZ", control->level, control->address);

    consume(sTO);
    expr_type = parse_expr();
    require_type(expr_type, TYPE_INT, line, column, "limite do for");
    emit2_int("ARMZ", current_mepa_level, limit_temp);

    if (accept(sSTEP)) {
        int sign = 1;
        if (accept(sSUBRAT)) {
            sign = -1;
        }
        if (current_token.category == sCTEINT) {
            step_value = sign * (int)strtol(current_token.lexeme, NULL, 10);
            if (step_value == 0) {
                diag_errorf(current_token.line, current_token.column, "step do for nao pode ser zero");
            }
            emit1_int("CRCT", step_value);
            consume(sCTEINT);
        } else if (current_token.category == sIDENTIF && sign > 0) {
            const Symbol *step_symbol;
            char step_name[64];
            int step_line = current_token.line;
            int step_column = current_token.column;
            copy_lexeme(&current_token, step_name, sizeof(step_name));
            consume(sIDENTIF);
            step_symbol = require_symbol(step_name, step_line, step_column);
            require_assignable_symbol(step_symbol, step_name, step_line, step_column);
            require_type(step_symbol->type, TYPE_INT, step_line, step_column, "step do for");
            emit2_int("CRVL", step_symbol->level, step_symbol->address);
            step_value = 1;
        } else {
            diag_errorf(current_token.line, current_token.column, "step do for deve ser inteiro ou identificador inteiro");
        }
        emit2_int("ARMZ", current_mepa_level, step_temp);
    } else {
        emit1_int("CRCT", 1);
        emit2_int("ARMZ", current_mepa_level, step_temp);
    }

    emit_label(start_label);
    emit2_int("CRVL", current_mepa_level, step_temp);
    emit1_int("CRCT", 0);
    emit0("CMAG");
    emit1("DSVF", negative_step_label);
    emit2_int("CRVL", control->level, control->address);
    emit2_int("CRVL", current_mepa_level, limit_temp);
    emit0("CMEG");
    emit1("DSVS", after_condition_label);
    emit_label(negative_step_label);
    emit2_int("CRVL", control->level, control->address);
    emit2_int("CRVL", current_mepa_level, limit_temp);
    emit0("CMAG");
    emit_label(after_condition_label);
    emit1("DSVF", end_label);

    consume(sDO);
    parse_embedded_command();

    emit2_int("CRVL", control->level, control->address);
    emit2_int("CRVL", current_mepa_level, step_temp);
    emit0("SOMA");
    emit2_int("ARMZ", control->level, control->address);
    emit1("DSVS", start_label);
    emit_label(end_label);
    free_temp_slots(2);

    if (require_semicolon && current_token.category == sSEMI) {
        consume(sSEMI);
    }
}

static void parse_loop(int require_semicolon) {
    char *start_label = novo_rotulo();
    char *end_label = novo_rotulo();
    DataType cond_type;

    consume(sLOOP);

    if (accept(sWHILE)) {
        emit_label(start_label);
        consume(sLPAR);
        cond_type = parse_expr();
        require_type(cond_type, TYPE_BOOL, current_token.line, current_token.column, "condicao do loop while");
        consume(sRPAR);
        emit1("DSVF", end_label);
        parse_embedded_command();
        emit1("DSVS", start_label);
        emit_label(end_label);
        if (require_semicolon && current_token.category == sSEMI) {
            consume(sSEMI);
        }
        return;
    }

    emit_label(start_label);
    parse_statement_list(sUNTIL, sEOF);
    consume(sUNTIL);
    consume(sLPAR);
    cond_type = parse_expr();
    require_type(cond_type, TYPE_BOOL, current_token.line, current_token.column, "condicao do loop until");
    consume(sRPAR);
    emit1("DSVF", start_label);
    emit_label(end_label);
    if (require_semicolon) {
        consume(sSEMI);
    }
}

static void parse_return(int require_semicolon) {
    DataType expr_type;

    if (current_routine_kind != SYM_FN) {
        diag_errorf(current_token.line, current_token.column, "ret so pode ser usado em funcao");
    }

    consume(sRET);
    expr_type = parse_expr();
    require_type(expr_type, current_return_type, current_token.line, current_token.column, "retorno da funcao");
    current_function_has_return = 1;
    emit2_int("ARMZ", current_mepa_level, current_return_addr);
    if (current_local_alloc > 0) {
        emit1_int("DMEM", current_local_alloc);
    }
    emit2_int("RTPR", current_mepa_level, current_param_count);

    if (require_semicolon) {
        consume(sSEMI);
    }
}

static const Symbol *parse_variable_reference(int require_declared, int emit_load) {
    char name[64];
    int line = current_token.line;
    int column = current_token.column;
    const Symbol *symbol;
    int indexed = 0;

    last_reference_indirect = 0;
    copy_lexeme(&current_token, name, sizeof(name));
    consume(sIDENTIF);

    symbol = require_declared ? require_symbol(name, line, column) : ts_lookup(name);
    if (symbol == NULL) {
        return NULL;
    }
    require_assignable_symbol(symbol, name, line, column);

    if (accept(sLBRACK)) {
        if (symbol->extra <= 0) {
            diag_errorf(line, column, "\"%s\" nao e vetor", name);
        }
        emit_vector_address_to(symbol, line, column, current_temp_addr);
        indexed = 1;
    }

    if (symbol->extra > 0 && !indexed) {
        diag_errorf(line, column, "vetor \"%s\" precisa ser indexado", name);
    }
    last_reference_indirect = indexed;

    if (emit_load) {
        if (indexed) {
            emit2_int("CRVI", current_mepa_level, current_temp_addr);
        } else {
            emit2_int("CRVL", symbol->level, symbol->address);
        }
    }

    return symbol;
}

static int parse_argument_list(const Symbol *routine) {
    int count = 0;

    while (1) {
        DataType arg_type;

        if (count >= routine->param_count) {
            diag_errorf(current_token.line,
                        current_token.column,
                        "sub-rotina \"%s\" recebeu argumentos demais",
                        routine->lexeme);
        }

        arg_type = parse_expr();
        require_type(arg_type,
                     routine->param_types[count],
                     current_token.line,
                     current_token.column,
                     "argumento de chamada");
        count++;

        if (!accept(sCOMMA)) {
            break;
        }
    }

    if (count != routine->param_count) {
        diag_errorf(current_token.line,
                    current_token.column,
                    "sub-rotina \"%s\" espera %d argumento(s), recebeu %d",
                    routine->lexeme,
                    routine->param_count,
                    count);
    }

    return count;
}

static void parse_when_condition(const char *body_label) {
    parse_when_item(body_label);
    while (accept(sCOMMA)) {
        parse_when_item(body_label);
    }
}

static void parse_when_item(const char *body_label) {
    int first = parse_range_int();
    if (accept(sPTOPTO)) {
        char *fail_label = novo_rotulo();
        int last = parse_range_int();
        emit2_int("CRVL", current_mepa_level, current_temp_addr);
        emit1_int("CRCT", first);
        emit0("CMAG");
        emit1("DSVF", fail_label);
        emit2_int("CRVL", current_mepa_level, current_temp_addr);
        emit1_int("CRCT", last);
        emit0("CMEG");
        emit1("DSVF", fail_label);
        emit1("DSVS", body_label);
        emit_label(fail_label);
        return;
    }

    {
        char *fail_label = novo_rotulo();
        emit2_int("CRVL", current_mepa_level, current_temp_addr);
        emit1_int("CRCT", first);
        emit0("CMIG");
        emit1("DSVF", fail_label);
        emit1("DSVS", body_label);
        emit_label(fail_label);
    }
}

static int parse_range_int(void) {
    int sign = 1;
    int value;

    if (accept(sSUBRAT)) {
        sign = -1;
    }
    value = (int)strtol(current_token.lexeme, NULL, 10);
    consume(sCTEINT);
    return sign * value;
}

static DataType parse_expr(void) {
    return parse_or_expr();
}

static DataType parse_or_expr(void) {
    DataType left = parse_and_expr();
    while (current_token.category == sOR) {
        int line = current_token.line;
        int column = current_token.column;
        consume(sOR);
        require_type(left, TYPE_BOOL, line, column, "operador logico");
        require_type(parse_and_expr(), TYPE_BOOL, line, column, "operador logico");
        emit0("DISJ");
        left = TYPE_BOOL;
    }
    return left;
}

static DataType parse_and_expr(void) {
    DataType left = parse_rel_expr();
    while (current_token.category == sAND) {
        int line = current_token.line;
        int column = current_token.column;
        consume(sAND);
        require_type(left, TYPE_BOOL, line, column, "operador logico");
        require_type(parse_rel_expr(), TYPE_BOOL, line, column, "operador logico");
        emit0("CONJ");
        left = TYPE_BOOL;
    }
    return left;
}

static DataType parse_rel_expr(void) {
    DataType left = parse_add_expr();
    while (current_token.category == sMENOR || current_token.category == sMENORIG ||
           current_token.category == sMAIOR || current_token.category == sMAIORIG ||
           current_token.category == sIGUAL || current_token.category == sDIFERENTE) {
        Category op = current_token.category;
        int line = current_token.line;
        int column = current_token.column;
        DataType right;

        next_token();
        right = parse_add_expr();
        if (!same_type(left, right)) {
            diag_errorf(line, column, "tipos incompativeis em comparacao");
        }
        if ((op == sMENOR || op == sMENORIG || op == sMAIOR || op == sMAIORIG) && left == TYPE_BOOL) {
            diag_errorf(line, column, "comparacao ordenada requer int ou char");
        }

        switch (op) {
            case sMENOR: emit0("CMME"); break;
            case sMAIOR: emit0("CMMA"); break;
            case sIGUAL: emit0("CMIG"); break;
            case sDIFERENTE: emit0("CMDG"); break;
            case sMENORIG: emit0("CMEG"); break;
            case sMAIORIG: emit0("CMAG"); break;
            default: break;
        }
        left = TYPE_BOOL;
    }
    return left;
}

static DataType parse_add_expr(void) {
    DataType left = parse_mul_expr();
    while (current_token.category == sSOMA || current_token.category == sSUBRAT) {
        Category op = current_token.category;
        int line = current_token.line;
        int column = current_token.column;
        DataType right;

        next_token();
        right = parse_mul_expr();
        require_type(left, TYPE_INT, line, column, "operador aritmetico");
        require_type(right, TYPE_INT, line, column, "operador aritmetico");
        emit0(op == sSOMA ? "SOMA" : "SUBT");
        left = TYPE_INT;
    }
    return left;
}

static DataType parse_mul_expr(void) {
    DataType left = parse_unary_expr();
    while (current_token.category == sMULT || current_token.category == sDIV) {
        Category op = current_token.category;
        int line = current_token.line;
        int column = current_token.column;
        DataType right;

        next_token();
        right = parse_unary_expr();
        require_type(left, TYPE_INT, line, column, "operador aritmetico");
        require_type(right, TYPE_INT, line, column, "operador aritmetico");
        emit0(op == sMULT ? "MULT" : "DIVI");
        left = TYPE_INT;
    }
    return left;
}

static DataType parse_unary_expr(void) {
    if (current_token.category == sNEG) {
        int line = current_token.line;
        int column = current_token.column;
        consume(sNEG);
        require_type(parse_unary_expr(), TYPE_BOOL, line, column, "negacao logica");
        emit0("NEGA");
        return TYPE_BOOL;
    }

    if (current_token.category == sSUBRAT) {
        int line = current_token.line;
        int column = current_token.column;
        consume(sSUBRAT);
        require_type(parse_unary_expr(), TYPE_INT, line, column, "sinal negativo");
        emit1_int("CRCT", -1);
        emit0("MULT");
        return TYPE_INT;
    }

    return parse_primary_expr();
}

static DataType parse_primary_expr(void) {
    switch (current_token.category) {
        case sCTEINT:
            emit1("CRCT", current_token.lexeme);
            next_token();
            return TYPE_INT;
        case sCTECHAR:
            emit1_int("CRCT", current_token.lexeme != NULL ? (unsigned char)current_token.lexeme[0] : 0);
            next_token();
            return TYPE_CHAR;
        case sSTRING:
            next_token();
            return TYPE_STRING;
        case sTRUE:
            emit1_int("CRCT", 1);
            next_token();
            return TYPE_BOOL;
        case sFALSE:
            emit1_int("CRCT", 0);
            next_token();
            return TYPE_BOOL;
        case sLPAR: {
            DataType type;
            consume(sLPAR);
            type = parse_expr();
            consume(sRPAR);
            return type;
        }
        case sIDENTIF: {
            char name[64];
            int line = current_token.line;
            int column = current_token.column;
            const Symbol *symbol;
            int indexed = 0;

            copy_lexeme(&current_token, name, sizeof(name));
            consume(sIDENTIF);

            if (current_token.category == sLPAR) {
                return parse_call_tail(name, line, column, 0, 1);
            }

            symbol = require_symbol(name, line, column);
            require_assignable_symbol(symbol, name, line, column);
            if (accept(sLBRACK)) {
                if (symbol->extra <= 0) {
                    diag_errorf(line, column, "\"%s\" nao e vetor", name);
                }
                emit_vector_address_to(symbol, line, column, current_temp_addr);
                indexed = 1;
            }
            if (symbol->extra > 0 && !indexed) {
                diag_errorf(line, column, "vetor \"%s\" precisa ser indexado", name);
            }
            if (indexed) {
                emit2_int("CRVI", current_mepa_level, current_temp_addr);
            } else {
                emit2_int("CRVL", symbol->level, symbol->address);
            }
            return symbol->type;
        }
        default:
            diag_errorf(current_token.line,
                        current_token.column,
                        "expressao invalida iniciando em %s \"%s\"",
                        token_category_name(current_token.category),
                        current_token.lexeme);
    }

    return TYPE_NONE;
}

static DataType token_to_type(Category category) {
    switch (category) {
        case sINT: return TYPE_INT;
        case sBOOL: return TYPE_BOOL;
        case sCHAR: return TYPE_CHAR;
        default: return TYPE_NONE;
    }
}

static const Symbol *require_symbol(const char *name, int line, int column) {
    const Symbol *symbol = ts_lookup(name);
    if (symbol == NULL) {
        diag_errorf(line, column, "identificador \"%s\" nao declarado", name);
    }
    return symbol;
}

static void require_assignable_symbol(const Symbol *symbol, const char *name, int line, int column) {
    if (symbol == NULL) {
        diag_errorf(line, column, "identificador \"%s\" nao declarado", name);
    }
    if (symbol->kind != SYM_VAR && symbol->kind != SYM_PARAM) {
        diag_errorf(line, column, "\"%s\" nao e variavel", name);
    }
}

static void require_type(DataType found, DataType expected, int line, int column, const char *context) {
    if (!same_type(found, expected)) {
        diag_errorf(line,
                    column,
                    "tipo incompativel em %s: esperado %s, encontrado %s",
                    context,
                    ts_data_type_name(expected),
                    ts_data_type_name(found));
    }
}

static int same_type(DataType a, DataType b) {
    return a == b && a != TYPE_NONE && b != TYPE_NONE;
}

static char *copy_lexeme(const Token *token, char *buffer, size_t size) {
    if (size == 0) {
        return buffer;
    }

    strncpy(buffer, token->lexeme != NULL ? token->lexeme : "", size - 1);
    buffer[size - 1] = '\0';
    return buffer;
}

static void emit0(const char *mnemonic) {
    gera_instr_mepa("", (char *)mnemonic, NULL, NULL);
}

static void emit1(const char *mnemonic, const char *param) {
    gera_instr_mepa("", (char *)mnemonic, (char *)param, NULL);
}

static void emit1_int(const char *mnemonic, int value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    emit1(mnemonic, buffer);
}

static void emit2_int(const char *mnemonic, int a, int b) {
    char p1[32];
    char p2[32];
    snprintf(p1, sizeof(p1), "%d", a);
    snprintf(p2, sizeof(p2), "%d", b);
    gera_instr_mepa("", (char *)mnemonic, p1, p2);
}

static void emit_label(const char *label) {
    gera_instr_mepa((char *)label, "NADA", NULL, NULL);
}

static void emit_string_literal(const char *text) {
    size_t i;
    for (i = 0; text[i] != '\0'; ++i) {
        emit1_int("CRCT", (unsigned char)text[i]);
        emit0("IMPR");
    }
}

static int symbol_storage_size(int extra) {
    return extra > 0 ? extra + 1 : 1;
}

static void add_vector_init(int level, int address, int size) {
    VectorInit *items = current_decl_is_global ? global_vector_inits : local_vector_inits;
    int *count = current_decl_is_global ? &global_vector_init_count : &local_vector_init_count;

    if (*count >= MAX_VECTOR_INITS) {
        diag_errorf(current_token.line, current_token.column, "limite interno de vetores excedido");
    }

    items[*count].level = level;
    items[*count].address = address;
    items[*count].size = size;
    (*count)++;
}

static void emit_vector_inits(VectorInit *items, int count) {
    int i;

    for (i = 0; i < count; ++i) {
        emit1_int("CRCT", items[i].size);
        emit2_int("ARMZ", items[i].level, items[i].address);
    }
}

static void emit_vector_address_to(const Symbol *symbol, int line, int column, int temp_addr) {
    DataType index_type;

    if (temp_addr < 0) {
        diag_errorf(line, column, "falha interna: vetor requer temporario MEPA");
    }

    emit2_int("CREN", symbol->level, symbol->address);
    index_type = parse_expr();
    require_type(index_type, TYPE_INT, line, column, "indice de vetor");
    emit0("SOMA");
    emit2_int("ARMZ", current_mepa_level, temp_addr);
    consume(sRBRACK);
}

static void reserve_temp_if_needed(void) {
    if (!program_needs_temp) {
        return;
    }

    if (current_mepa_level == 0) {
        current_temp_pool_base = global_next_addr + local_next_addr;
    } else {
        current_temp_pool_base = local_next_addr;
    }
    current_temp_addr = current_temp_pool_base;
    current_temp2_addr = current_temp_pool_base + 1;
    current_temp_pool_next = 2;
    local_next_addr += TEMP_POOL_SIZE;
    current_local_alloc += TEMP_POOL_SIZE;
}

static int alloc_temp_slot(int line, int column) {
    if (current_temp_pool_base < 0 || current_temp_pool_next >= TEMP_POOL_SIZE) {
        diag_errorf(line, column, "limite interno de temporarios MEPA excedido");
    }
    return current_temp_pool_base + current_temp_pool_next++;
}

static void free_temp_slots(int count) {
    current_temp_pool_next -= count;
    if (current_temp_pool_next < 2) {
        current_temp_pool_next = 2;
    }
}
