#include <preprocessor.h>
#include <lexer.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.ent>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];

    struct preprocessor pp = preprocessor_create(filename, false);
    if (!pp.preprocessed_file) {
        fprintf(stderr, "Preprocessing failed.\n");
        return EXIT_FAILURE;
    }

    printf("--- Tokens ---\n");
    for (size_t i = 0; i < pp.tokens.size; i++) {
        const token_t *token = &pp.tokens.data[i];
        printf("[%zu:%zu] %d: %.*s\n", token->line, token->column, token->type, (int)token->length, token->lexeme);
    }

    preprocessor_destroy(&pp);

    printf("Processing complete.\n");
    return EXIT_SUCCESS;
}
