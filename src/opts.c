#include <stdio.h>
#include <string.h>

#include "opts.h"

int opts_parse(int argc, char **argv, Options *options) {
    int i;

    options->source_path = NULL;
    options->emit_tokens = 0;
    options->emit_symtab = 0;
    options->emit_trace = 0;

    if (argc < 2) {
        return 0;
    }

    options->source_path = argv[1];

    for (i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--tokens") == 0) {
            options->emit_tokens = 1;
        } else if (strcmp(argv[i], "--symtab") == 0) {
            options->emit_symtab = 1;
        } else if (strcmp(argv[i], "--trace") == 0) {
            options->emit_trace = 1;
        } else {
            return 0;
        }
    }

    return 1;
}

void opts_print_usage(const char *program_name) {
    fprintf(stderr, "uso: %s <arquivo.sal> [--tokens] [--symtab] [--trace]\n", program_name);
}
