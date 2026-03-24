#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "token.h"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: %s <arquivo.sal>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];

    FILE *file = fopen(input_file, "r");
    if (!file) {
        printf("Erro ao abrir arquivo: %s\n", input_file);
        return 1;
    }

    /* cria nome do arquivo de saída (.tk) */
    char output_file[256];
    strcpy(output_file, input_file);

    char *dot = strrchr(output_file, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
    strcat(output_file, ".tk");

    FILE *out = fopen(output_file, "w");
    if (!out) {
        printf("Erro ao criar arquivo de saída\n");
        fclose(file);
        return 1;
    }

    Lexer lexer = lex_init(file);
    Token token;

    while (1) {
        token = lex_next(&lexer);

        /* escreve no formato: linha  categoria  "lexema" */
        fprintf(out, "%d  %d  \"%s\"\n",
                token.line,
                token.category,
                token.lexeme ? token.lexeme : "");

        if (token.category == sEOF) {
            break;
        }

        /* libera memória do lexema */
        if (token.lexeme != NULL) {
            free(token.lexeme);
        }
    }

    fclose(file);
    fclose(out);

    printf("Tokens gerados em: %s\n", output_file);

    return 0;
}