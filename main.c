#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "token.h"
#include "parser.h"
#include "symtab.h"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: %s <arquivo.sal>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];

    int do_tokens = (argc >= 3 && strcmp(argv[2], "--tokens") == 0);

    FILE *file = fopen(input_file, "r");
    if (!file) {
        printf("Erro ao abrir arquivo: %s\n", input_file);
        return 1;
    }

    FILE *out = NULL;
    char output_file[256];

    if (do_tokens) {
        strcpy(output_file, input_file);

        char *dot = strrchr(output_file, '.');
        if (dot != NULL) {
            *dot = '\0';
        }
        strcat(output_file, ".tk");

        out = fopen(output_file, "w");
        if (!out) {
            printf("Erro ao criar arquivo de saída\n");
            fclose(file);
            return 1;
        }
    }

    Lexer lexer = lex_init(file);

    if (!do_tokens) {
        /* modo parser */
        ts_init();
        parse_program(&lexer);
    } else {
        /* modo geração de tokens */
        Token token;
        while (1) {
            token = lex_next(&lexer);

            fprintf(out, "%d  %d  \"%s\"\n",
                    token.line,
                    token.category,
                    token.lexeme ? token.lexeme : "");

            if (token.category == sEOF) {
                if (token.lexeme != NULL) free(token.lexeme);
                break;
            }

            if (token.lexeme != NULL) {
                free(token.lexeme);
            }
        }
    }

    fclose(file);

    if (do_tokens) {
        fclose(out);
        printf("Tokens gerados em: %s\n", output_file);
    } else {
        printf("Parsing concluído com sucesso.\n");
    }

    return 0;
}