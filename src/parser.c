#include <parser.h>
#include <ast.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr uint64_t FNV_OFFSET_BASIS = 1469598103934665603ULL;
constexpr uint64_t FNV_PRIME        = 1099511628211ULL;
static uint64_t parser_fnv1a_hash_string(char** string) {
    const char* str =* string;
    uint64_t hash = FNV_OFFSET_BASIS;
    for (;* str; ++str) {
        hash ^= (unsigned char)(*str);
        hash* = FNV_PRIME;
    }
    return hash;
}
static int parser_strcmp_wrapper(char** a, char** b) {
    return strcmp(*a,* b);
}

DEFINE_HASHMAP_FUNCTIONS(
    char*,
    char*,
    strmap,
    parser_fnv1a_hash_string,
    parser_strcmp_wrapper,
    1,
    1
)
DEFINE_VECTOR_FUNCTIONS(type_field_t)

const char* base_types[13] = {
    "void", "varargs",
    "byte", "word", "dword", "qword",
    "sbyte", "sword", "sdword", "sqword",
    "single", "double", /* not a base type but used for mappings: */ "struct"
};

static error_context_t make_error_context(const parser_t* p) {
    const token_t* t = nullptr;
    if (p->current < p->tokens->size) {
        t = &p->tokens->data[p->current];
    }
    error_context_t ctx;
    ctx.module = "parser";
    ctx.file = (t && t->lexeme) ? t->lexeme : "<unknown file>";
    ctx.source_line = nullptr;
    ctx.line = t ? (int)t->line : -1;
    ctx.column = t ? (int)t->column : -1;
    return ctx;
}

static token_t* peek(const parser_t* p) {
    if (p->current < p->tokens->size) {
        return &p->tokens->data[p->current];
    }
    return nullptr;
}

static bool check(const parser_t* p, const enum token_type type) {
    const token_t* t = peek(p);
    return (t != nullptr && t->type == type);
}

static bool match(parser_t* p, const enum token_type type) {
    if (check(p, type)) {
        p->current++;
        return true;
    }
    return false;
}

static token_t* consume(parser_t* p, const enum token_type type, const char* error_msg) {
    token_t* t = peek(p);
    if (t == nullptr) {
        const error_context_t ctx = make_error_context(p);
        compiler_error(&ctx, "Unexpected end of file. %s", error_msg);
        p->error_count++;
        return nullptr;
    }
    if (t->type == type) {
        p->current++;
        return t;
    }
    const error_context_t ctx = make_error_context(p);
    compiler_error(&ctx, "Expected token '%d', got '%d'. %s", type, t->type, error_msg);
    p->error_count++;
    return nullptr;
}

static token_t* advance(parser_t* p) {
    token_t* t = peek(p);
    if (t == nullptr) {
        const error_context_t ctx = make_error_context(p);
        compiler_error(&ctx, "Unexpected end of file.");
        p->error_count++;
        return nullptr;
    }
    p->current++;
    return t;
}

static struct ast_common* create_ast_node(const parser_t* p, const enum ast_code code) {
    struct ast_common* node = ast_create_node(code);
    if (!node) return nullptr;

    const token_t* t = peek(p);
    if (t) {
        node->filename = t->lexeme;
        node->line     = (int)t->line;
        node->column   = (int)t->column;
    }
    return node;
}

void parser_init(parser_t* p, token_t_vector_t* tokens) {
    p->tokens = tokens;
    p->current = 0;
    p->error_count = 0;
    strmap_init(&p->type_to_base_mapping, 10);
}

void parser_destroy(parser_t* p) {
    p->tokens = nullptr;
    p->current = 0;
    p->error_count = 0;
    strmap_destroy(&p->type_to_base_mapping);
}

/* ------------------------------------------------------------------- */

static struct ast_node_list* parse_function_parameters(parser_t* p);

/* ------------------------------------------------------------------- */

struct ast_translation_unit* parse_translation_unit(parser_t* p) {
    struct ast_translation_unit* unit =
        (struct ast_translation_unit*) ast_create_node(AST_CODE_TRANSLATION_UNIT);
    if (!unit) return nullptr;

    unit->decls = ast_create_node_list();

    /* Repeatedly parse declarations or definitions until out of tokens. */
    while (p->current < p->tokens->size) {
        token_t* tk = peek(p);
        if (!tk) break;

        switch (tk->type) {
            case TOKEN_FUNCTION: {
                advance(p);
                const token_t* nameToken = advance(p);
                const char* name = nameToken->lexeme;
                struct ast_node_list* params = parse_function_parameters(p);
            }
            case TOKEN_EXTERN: {

            }
            case TOKEN_STRUCT:
            case TOKEN_ENUM:
            case TOKEN_UNION: {

            }
            case TOKEN_TYPEDEF: {

            }
            case TOKEN_ASM: {

            }
            default: {
                /* Check if it is a type */
            }
        }
    }

    return unit;
}
