#include <stdio.h>
#include <stdlib.h>

#include "diag.h"
#include "lex.h"
#include "log.h"
#include "opts.h"
#include "parser.h"
#include "symtab.h"

int main(int argc, char **argv) {
    /* main so coordena os modulos */
    Options options;
    FILE *source = NULL;
    FILE *token_log = NULL;
    FILE *trace_log = NULL;
    FILE *symtab_log = NULL;
    char token_path[512];
    char trace_path[512];
    char symtab_path[512];
    Lexer lexer;

    /* le os argumentos da linha de comando */
    if (!opts_parse(argc, argv, &options)) {
        opts_print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /* abre o arquivo fonte */
    source = fopen(options.source_path, "r");
    if (source == NULL) {
        fprintf(stderr, "erro: nao foi possivel abrir \"%s\"\n", options.source_path);
        return EXIT_FAILURE;
    }

    /* cria os logs so quando o usuario pedir */
    if (options.emit_tokens) {
        token_log = log_open_with_extension(options.source_path, ".tk", token_path, sizeof(token_path));
        if (token_log == NULL) {
            fprintf(stderr, "erro: nao foi possivel criar log de tokens\n");
            fclose(source);
            return EXIT_FAILURE;
        }
    }

    if (options.emit_trace) {
        trace_log = log_open_with_extension(options.source_path, ".trc", trace_path, sizeof(trace_path));
        if (trace_log == NULL) {
            fprintf(stderr, "erro: nao foi possivel criar log de rastreamento\n");
            fclose(source);
            if (token_log != NULL) {
                fclose(token_log);
            }
            return EXIT_FAILURE;
        }
    }

    if (options.emit_symtab) {
        symtab_log = log_open_with_extension(options.source_path, ".ts", symtab_path, sizeof(symtab_path));
        if (symtab_log == NULL) {
            fprintf(stderr, "erro: nao foi possivel criar log da tabela de simbolos\n");
            fclose(source);
            if (token_log != NULL) {
                fclose(token_log);
            }
            if (trace_log != NULL) {
                fclose(trace_log);
            }
            return EXIT_FAILURE;
        }
    }

    /* inicializa os modulos principais */
    diag_init(trace_log);
    ts_init();
    lex_init(&lexer, source, token_log);

    /* inicia a analise do programa */
    parse_program(&lexer);

    if (symtab_log != NULL) {
        ts_dump(symtab_log);
    }

    /* fecha tudo no final */
    lex_destroy(&lexer);
    ts_destroy();
    diag_shutdown();

    fclose(source);
    if (token_log != NULL) {
        fclose(token_log);
    }
    if (trace_log != NULL) {
        fclose(trace_log);
    }
    if (symtab_log != NULL) {
        fclose(symtab_log);
    }

    return EXIT_SUCCESS;
}
