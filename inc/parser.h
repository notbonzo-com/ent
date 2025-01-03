#ifndef PARSER_H
#define PARSER_H

#include <lexer.h>

typedef struct parser {
    token_t_vector_t *tokens;
    size_t current;
    size_t error_count;

} parser_t;

void parser_init(parser_t *p, token_t_vector_t *tokens);
struct ast_translation_unit* parse_translation_unit(parser_t *p);
void parser_destroy(parser_t *p);

#endif /* PARSER_H */
