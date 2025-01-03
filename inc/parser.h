#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <lexer.h>
#include <stdint.h>
#include <map.h>

DECLARE_HASHMAP_TYPE(char*, char*, strmap)

typedef struct {
    char *name;
    struct type *type;
    size_t offset;
} type_field_t;

DECLARE_VECTOR_TYPE(type_field_t)

struct type {
    char *name;
    int pointer; /* 0 if not pointer, otherwise number of nested pointing's */
    bool is_struct;
    type_field_t_vector_t fields; /* fileds->data is nullptr if not a struct */
};

typedef struct parser {
    token_t_vector_t *tokens;
    size_t current;
    size_t error_count;

    /* types */
    strmap_t type_to_base_mapping;

    /* todo have the lexer store filename and the source line in the token (ideally in a memory respecting way) */
} parser_t;

void parser_init(parser_t *p, token_t_vector_t *tokens);
struct ast_translation_unit* parse_translation_unit(const parser_t *p);
void parser_destroy(parser_t *p);

#endif /* PARSER_H */
