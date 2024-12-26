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

    struct preprocessor pp = preprocessor_create(filename);
    if (!pp.preprocessed_file) {
        fprintf(stderr, "Preprocessing failed.\n");
        return EXIT_FAILURE;
    }

    printf("--- Preprocessed Output ---\n%s\n", pp.preprocessed_file);

    struct lexer lex;
    lexer_init(&lex, pp.preprocessed_file, filename);

    printf("--- Tokens ---\n");
    for (size_t i = 0; i < lex.tokens.size; i++) {
        token_t *token = &lex.tokens.data[i];
        printf("[%zu:%zu] %d: %.*s\n", token->line, token->column, token->type, (int)token->length, token->lexeme);
    }

    lexer_destroy(&lex);
    preprocessor_destroy(&pp);

    printf("Processing complete.\n");
    return EXIT_SUCCESS;
}
