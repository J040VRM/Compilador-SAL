#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag.h"
#include "parser.h"
#include "symtab.h"

static Lexer *parser_lexer = NULL;
static Token current_token;

/* cada funcao representa uma parte da gramatica */
static void next_token(void);
static void consume(Category expected);
static int accept(Category category);
static Token peek_token(void);
static void trace_enter(const char *name);
static void trace_exit(const char *name);

static void parse_module(void);
static void parse_globals(void);
static void parse_declaration_list(void);
static void parse_declaration(void);
static DataType parse_type(void);
static void parse_subroutine_list(void);
static void parse_subroutine(void);
static void parse_main(void);
static int parse_params_and_count(void);
static int parse_optional_locals(const char *scope_name);
static void parse_block(int create_scope);
static void parse_statement_list(Category terminator_a, Category terminator_b);
static void parse_statement(void);
static void parse_embedded_command(void);
static void parse_assignment_or_call(int require_semicolon);
static void parse_assignment_tail(const char *identifier, int line, int column, int require_semicolon);
static void parse_call_tail(const char *identifier, int line, int column, int require_semicolon);
static void parse_scan(int require_semicolon);
static void parse_print(int require_semicolon);
static void parse_if(int require_semicolon);
static void parse_match(int require_semicolon);
static void parse_for(int require_semicolon);
static void parse_loop(int require_semicolon);
static void parse_return(int require_semicolon);
static void parse_variable_reference(int require_declared);
static void parse_argument_list(void);
static void parse_when_condition(void);
static void parse_when_item(void);
static void parse_range_int(void);

static void parse_expr(void);
static void parse_or_expr(void);
static void parse_and_expr(void);
static void parse_rel_expr(void);
static void parse_add_expr(void);
static void parse_mul_expr(void);
static void parse_unary_expr(void);
static void parse_primary_expr(void);

static DataType token_to_type(Category category);
static void require_symbol(const char *name, int line, int column);
static char *copy_lexeme(const Token *token, char *buffer, size_t size);

void parse_program(Lexer *lexer) {
    /* ponto de entrada do parser */
    parser_lexer = lexer;
    next_token();
    parse_module();
    token_free(&current_token);
}

static void next_token(void) {
    /* consome o token atual e pede o proximo ao lexer */
    token_free(&current_token);
    current_token = lex_next(parser_lexer);
}

static void consume(Category expected) {
    /* garante que o token atual eh o esperado */
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

    /* olha o proximo token sem alterar o estado real do parser */
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

static void parse_module(void) {
    /* programa SAL comeca com module ... ; */
    trace_enter("program");

    consume(sMODULE);
    consume(sIDENTIF);
    consume(sSEMI);

    if (accept(sGLOBALS)) {
        parse_globals();
    }

    parse_subroutine_list();
    parse_main();
    consume(sEOF);

    trace_exit("program");
}

static void parse_globals(void) {
    trace_enter("globals");
    parse_declaration_list();
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
    int count = 0;
    DataType type;

    trace_enter("decl");

    /* aceita lista de ids simples ou vetores */
    do {
        copy_lexeme(&current_token, names[count], sizeof(names[count]));
        consume(sIDENTIF);

        extras[count] = 0;
        if (accept(sLBRACK)) {
            if (current_token.category != sCTEINT) {
                diag_syntax_expected(sCTEINT, &current_token);
            }
            extras[count] = (int)strtol(current_token.lexeme, NULL, 10);
            consume(sCTEINT);
            consume(sRBRACK);
        }

        count++;
    } while (accept(sCOMMA));

    consume(sCOLON);
    type = parse_type();
    consume(sSEMI);

    {
        int i;
        for (i = 0; i < count; ++i) {
            if (!ts_insert(names[i], SYM_VAR, type, extras[i])) {
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
    char name[64];
    char scope_name[96];
    int param_count;
    int has_locals;

    trace_enter("subroutine");

    /* entra em proc ou fn */
    next_token();
    if (current_token.category == sMAIN) {
        diag_errorf(current_token.line, current_token.column, "proc main deve ser declarado por ultimo");
    }

    copy_lexeme(&current_token, name, sizeof(name));
    if (!ts_insert(name, kind, TYPE_NONE, 0)) {
        diag_errorf(current_token.line, current_token.column, "sub-rotina \"%s\" ja declarada", name);
    }
    consume(sIDENTIF);
    consume(sLPAR);

    snprintf(scope_name, sizeof(scope_name), "%s:%s", kind == SYM_FN ? "fn" : "proc", name);
    /* parametros e locals ficam dentro do escopo da sub-rotina */
    ts_enter_named_scope(scope_name);
    param_count = parse_params_and_count();
    consume(sRPAR);

    if (routine_token == sFN) {
        consume(sCOLON);
        return_type = parse_type();
    }

    if (!ts_update_global(name, kind, return_type, param_count)) {
        diag_errorf(current_token.line, current_token.column, "falha ao atualizar assinatura de \"%s\"", name);
    }

    has_locals = parse_optional_locals(scope_name);
    parse_block(1);
    if (has_locals) {
        ts_exit_scope();
    }
    ts_exit_scope();

    trace_exit("subroutine");
}

static void parse_main(void) {
    int has_locals;

    trace_enter("main");

    consume(sPROC);
    consume(sMAIN);
    consume(sLPAR);
    consume(sRPAR);

    if (!ts_insert("main", SYM_PROC, TYPE_NONE, 0)) {
        diag_errorf(current_token.line, current_token.column, "sub-rotina \"main\" ja declarada");
    }

    ts_enter_named_scope("proc:main");
    has_locals = parse_optional_locals("proc:main");
    parse_block(1);
    if (has_locals) {
        ts_exit_scope();
    }
    ts_exit_scope();

    trace_exit("main");
}

static int parse_params_and_count(void) {
    int count = 0;

    trace_enter("params");

    if (current_token.category == sIDENTIF) {
        while (1) {
            char name[64];
            DataType type;

            copy_lexeme(&current_token, name, sizeof(name));
            consume(sIDENTIF);
            consume(sCOLON);
            type = parse_type();

            if (!ts_insert(name, SYM_PARAM, type, 0)) {
                diag_errorf(current_token.line, current_token.column, "parametro \"%s\" ja declarado", name);
            }
            count++;

            if (!accept(sCOMMA)) {
                break;
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

    /* cada start ... end pode abrir um bloco proprio */
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

    /* escolhe o tipo de comando pelo token atual */
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
        parse_call_tail(name, line, column, require_semicolon);
    } else {
        parse_assignment_tail(name, line, column, require_semicolon);
    }
}

static void parse_assignment_tail(const char *identifier, int line, int column, int require_semicolon) {
    require_symbol(identifier, line, column);

    if (accept(sLBRACK)) {
        parse_expr();
        consume(sRBRACK);
    }

    consume(sATRIB);
    parse_expr();

    if (require_semicolon) {
        consume(sSEMI);
    }
}

static void parse_call_tail(const char *identifier, int line, int column, int require_semicolon) {
    const Symbol *symbol = ts_lookup(identifier);

    if (symbol == NULL || (symbol->kind != SYM_PROC && symbol->kind != SYM_FN)) {
        diag_errorf(line, column, "sub-rotina \"%s\" nao declarada", identifier);
    }

    consume(sLPAR);
    if (current_token.category != sRPAR) {
        parse_argument_list();
    }
    consume(sRPAR);

    if (require_semicolon) {
        consume(sSEMI);
    }
}

static void parse_scan(int require_semicolon) {
    consume(sSCAN);
    consume(sLPAR);
    parse_variable_reference(1);
    consume(sRPAR);
    if (require_semicolon) {
        consume(sSEMI);
    }
}

static void parse_print(int require_semicolon) {
    consume(sPRINT);
    consume(sLPAR);
    parse_expr();
    while (accept(sCOMMA)) {
        parse_expr();
    }
    consume(sRPAR);
    if (require_semicolon) {
        consume(sSEMI);
    }
}

static void parse_if(int require_semicolon) {
    consume(sIF);
    consume(sLPAR);
    parse_expr();
    consume(sRPAR);
    parse_embedded_command();

    if (accept(sELSE)) {
        parse_embedded_command();
    }

    if (require_semicolon && current_token.category == sSEMI) {
        consume(sSEMI);
    }
}

static void parse_match(int require_semicolon) {
    int when_count = 0;

    /* match precisa de pelo menos um when */
    consume(sMATCH);
    consume(sLPAR);
    parse_expr();
    consume(sRPAR);

    while (accept(sWHEN)) {
        when_count++;
        parse_when_condition();
        consume(sIMPLIC);
        parse_embedded_command();
        consume(sSEMI);
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

    consume(sEND);
    if (require_semicolon && current_token.category == sSEMI) {
        consume(sSEMI);
    }
}

static void parse_for(int require_semicolon) {
    char control_name[64];
    int line;
    int column;

    /* for usa uma variavel de controle ja declarada */
    consume(sFOR);
    copy_lexeme(&current_token, control_name, sizeof(control_name));
    line = current_token.line;
    column = current_token.column;
    consume(sIDENTIF);
    require_symbol(control_name, line, column);

    if (accept(sLBRACK)) {
        parse_expr();
        consume(sRBRACK);
    }

    consume(sATRIB);
    parse_expr();
    consume(sTO);
    parse_expr();

    if (accept(sSTEP)) {
        parse_expr();
    }

    consume(sDO);
    parse_embedded_command();

    if (require_semicolon && current_token.category == sSEMI) {
        consume(sSEMI);
    }
}

static void parse_loop(int require_semicolon) {
    consume(sLOOP);

    /* loop while e loop until compartilham a palavra loop */
    if (accept(sWHILE)) {
        consume(sLPAR);
        parse_expr();
        consume(sRPAR);
        parse_embedded_command();
        if (require_semicolon && current_token.category == sSEMI) {
            consume(sSEMI);
        }
        return;
    }

    parse_statement_list(sUNTIL, sEOF);
    consume(sUNTIL);
    consume(sLPAR);
    parse_expr();
    consume(sRPAR);
    if (require_semicolon) {
        consume(sSEMI);
    }
}

static void parse_return(int require_semicolon) {
    consume(sRET);
    parse_expr();
    if (require_semicolon) {
        consume(sSEMI);
    }
}

static void parse_variable_reference(int require_declared) {
    char name[64];
    int line = current_token.line;
    int column = current_token.column;

    copy_lexeme(&current_token, name, sizeof(name));
    consume(sIDENTIF);

    if (require_declared) {
        require_symbol(name, line, column);
    }

    if (accept(sLBRACK)) {
        parse_expr();
        consume(sRBRACK);
    }
}

static void parse_argument_list(void) {
    parse_expr();
    while (accept(sCOMMA)) {
        parse_expr();
    }
}

static void parse_when_condition(void) {
    parse_when_item();
    while (accept(sCOMMA)) {
        parse_when_item();
    }
}

static void parse_when_item(void) {
    parse_range_int();
    if (accept(sPTOPTO)) {
        parse_range_int();
    }
}

static void parse_range_int(void) {
    accept(sSUBRAT);
    consume(sCTEINT);
}

static void parse_expr(void) {
    parse_or_expr();
}

static void parse_or_expr(void) {
    parse_and_expr();
    while (accept(sOR)) {
        parse_and_expr();
    }
}

static void parse_and_expr(void) {
    parse_rel_expr();
    while (accept(sAND)) {
        parse_rel_expr();
    }
}

static void parse_rel_expr(void) {
    parse_add_expr();
    while (current_token.category == sMENOR || current_token.category == sMENORIG ||
           current_token.category == sMAIOR || current_token.category == sMAIORIG ||
           current_token.category == sIGUAL || current_token.category == sDIFERENTE) {
        next_token();
        parse_add_expr();
    }
}

static void parse_add_expr(void) {
    parse_mul_expr();
    while (current_token.category == sSOMA || current_token.category == sSUBRAT) {
        next_token();
        parse_mul_expr();
    }
}

static void parse_mul_expr(void) {
    parse_unary_expr();
    while (current_token.category == sMULT || current_token.category == sDIV) {
        next_token();
        parse_unary_expr();
    }
}

static void parse_unary_expr(void) {
    if (current_token.category == sNEG || current_token.category == sSUBRAT) {
        next_token();
        parse_unary_expr();
        return;
    }

    parse_primary_expr();
}

static void parse_primary_expr(void) {
    /* parte mais basica das expressoes */
    switch (current_token.category) {
        case sCTEINT:
        case sCTECHAR:
        case sSTRING:
        case sTRUE:
        case sFALSE:
            next_token();
            return;
        case sLPAR:
            consume(sLPAR);
            parse_expr();
            consume(sRPAR);
            return;
        case sIDENTIF: {
            char name[64];
            int line = current_token.line;
            int column = current_token.column;

            copy_lexeme(&current_token, name, sizeof(name));
            consume(sIDENTIF);

            if (current_token.category == sLPAR) {
                const Symbol *symbol = ts_lookup(name);

                if (symbol == NULL) {
                    diag_errorf(line, column, "sub-rotina \"%s\" nao declarada", name);
                }
                /* so funcao pode aparecer dentro de expressao */
                if (symbol->kind != SYM_FN) {
                    diag_errorf(line, column, "procedimento \"%s\" nao pode ser usado em expressao", name);
                }
                parse_call_tail(name, line, column, 0);
            } else {
                require_symbol(name, line, column);
                if (accept(sLBRACK)) {
                    parse_expr();
                    consume(sRBRACK);
                }
            }
            return;
        }
        default:
            diag_errorf(current_token.line,
                        current_token.column,
                        "expressao invalida iniciando em %s \"%s\"",
                        token_category_name(current_token.category),
                        current_token.lexeme);
    }
}

static DataType token_to_type(Category category) {
    switch (category) {
        case sINT: return TYPE_INT;
        case sBOOL: return TYPE_BOOL;
        case sCHAR: return TYPE_CHAR;
        default: return TYPE_NONE;
    }
}

static void require_symbol(const char *name, int line, int column) {
    if (ts_lookup(name) == NULL) {
        diag_errorf(line, column, "identificador \"%s\" nao declarado", name);
    }
}

static char *copy_lexeme(const Token *token, char *buffer, size_t size) {
    if (size == 0) {
        return buffer;
    }

    strncpy(buffer, token->lexeme != NULL ? token->lexeme : "", size - 1);
    buffer[size - 1] = '\0';
    return buffer;
}
