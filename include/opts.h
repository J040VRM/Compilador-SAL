#ifndef OPTS_H
#define OPTS_H

typedef struct {
    const char *source_path;
    int emit_tokens;
    int emit_symtab;
    int emit_trace;
} Options;

int opts_parse(int argc, char **argv, Options *options);
void opts_print_usage(const char *program_name);

#endif
