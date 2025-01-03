#include <parser.h>
#include <ast.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* base_types[13] = {
    "void", "varargs",
    "byte", "word", "dword", "qword",
    "sbyte", "sword", "sdword", "sqword",
    "single", "double", /* not a base type but used for mappings: */ "struct"
};

static token_t* peek(const parser_t* p)
{
    if (p->current < p->tokens->size) {
        return &p->tokens->data[p->current];
    }
    return nullptr;
}

#if 0
static error_context_t make_error_context(const parser_t* p)
{
    const token_t* t = nullptr;
    if (p->current < p->tokens->size) {
        t = &p->tokens->data[p->current];
    }
    error_context_t ctx;
    ctx.module = "parser";
    ctx.file = /*(t && t->filename) ? t->filename :*/ "<unknown file>";
    ctx.source_line = nullptr;
    ctx.line = t ? (int)t->line : -1;
    ctx.column = t ? (int)t->column : -1;
    return ctx;
}

static bool check(const parser_t* p, const enum token_type type)
{
    const token_t* t = peek(p);
    return (t != nullptr && t->type == type);
}

static bool match(parser_t* p, const enum token_type type)
{
    if (check(p, type)) {
        p->current++;
        return true;
    }
    return false;
}

static token_t* consume(parser_t* p, const enum token_type type, const char* error_msg)
{
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

static token_t* advance(parser_t* p)
{
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
#endif

void parser_init(parser_t* p, token_t_vector_t* tokens)
{
    p->tokens = tokens;
    p->current = 0;
    p->error_count = 0;
}

void parser_destroy(parser_t* p)
{
    p->tokens = nullptr;
    p->current = 0;
    p->error_count = 0;
}

/* ------------------------------------------------------------------- */

/* ------------------------------------------------------------------- */

struct ast_translation_unit* parse_translation_unit(parser_t* p)
{
    struct ast_translation_unit* unit =
        (struct ast_translation_unit*) ast_create_node(AST_CODE_TRANSLATION_UNIT);
    if (!unit) return nullptr;

    unit->decls = ast_create_node_list();

    while (p->current < p->tokens->size) {
        token_t* tk = peek(p);
        if (!tk) break;

        switch (tk->type) {
            case TOKEN_FUNCTION: {
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
