#ifndef GERADOR_H
#define GERADOR_H

#include <stdio.h>

void gerador_init(FILE *out);
void gerador_shutdown(void);
void gera_instr_mepa(char *rotulo, char *mnemonico, char *parametro1, char *parametro2);
char *novo_rotulo(void);

#endif
