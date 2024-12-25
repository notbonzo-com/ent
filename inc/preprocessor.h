#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct preprocessor_define {
    char *name;
    char *value;
};

struct preprocessor_conditional_state {
    bool in_true_block;
    bool condition_met;
};

struct preprocessor {
    const char *filename;
    FILE *file;
    char *preprocessed_file;
    char *line;
    char *header_content;
    char **includes;
    size_t include_count;
    struct preprocessor_define *defines;
    size_t define_count;
    struct preprocessor_conditional_state *conditional_stack;
    size_t conditional_stack_size;
    size_t conditional_stack_capacity;

    bool in_header_block;
    int  brace_balance;
    size_t current_line;
    size_t current_column;
};

#ifdef __cplusplus
extern "C" {
#endif

struct preprocessor preprocessor_create(const char *filename);
void preprocessor_destroy(struct preprocessor *preprocessor);

#ifdef __cplusplus
}
#endif

#endif /* PREPROCESSOR_H */
