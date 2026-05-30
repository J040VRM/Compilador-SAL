#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gerador.h"

static FILE *mepa_out = NULL;
static int label_counter = 0;

void gerador_init(FILE *out) {
    mepa_out = out;
    label_counter = 0;
}

void gerador_shutdown(void) {
    mepa_out = NULL;
}

void gera_instr_mepa(char *rotulo, char *mnemonico, char *parametro1, char *parametro2) {
    if (mepa_out == NULL || mnemonico == NULL) {
        return;
    }

    if (rotulo != NULL && rotulo[0] != '\0') {
        fprintf(mepa_out, "%s:\t", rotulo);
    } else {
        fprintf(mepa_out, "\t");
    }

    fprintf(mepa_out, "%s", mnemonico);

    if (parametro1 != NULL && parametro1[0] != '\0') {
        fprintf(mepa_out, " %s", parametro1);
        if (parametro2 != NULL && parametro2[0] != '\0') {
            fprintf(mepa_out, ",%s", parametro2);
        }
    }

    fprintf(mepa_out, "\n");
}

char *novo_rotulo(void) {
    char buffer[32];
    char *label;

    snprintf(buffer, sizeof(buffer), "L%d", ++label_counter);
    label = (char *)malloc(strlen(buffer) + 1);
    if (label == NULL) {
        return NULL;
    }

    strcpy(label, buffer);
    return label;
}
