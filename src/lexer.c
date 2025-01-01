#include <lexer.h>
#include <error.h>
#include <vector.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <safemem.h>

DEFINE_VECTOR_FUNCTIONS(token_t)

static void lexer_scan_all(struct lexer *lexer);
static void lexer_skip_whitespace_and_comments(struct lexer *lexer);

static bool lexer_is_at_end(const struct lexer *lexer)
{
    return lexer->source[lexer->position] == '\0';
}

static char lexer_peek(const struct lexer *lexer)
{
    if (lexer_is_at_end(lexer)) {
        return '\0';
    }
    return lexer->source[lexer->position];
}

static const char* lexer_get_current_line(const struct lexer *lexer) {
    if (lexer->line == 0 || lexer->line > lexer->line_count) {
        return nullptr;
    }

    const size_t start = lexer->line_starts[lexer->line - 1];
    const size_t end = lexer->line < lexer->line_count
                           ? lexer->line_starts[lexer->line] - 1
                           : strlen(lexer->source);

    const size_t length = end - start;
    const char *line = strndup(&lexer->source[start], length);
    return line;
}

static void lexer_track_newline(struct lexer *lexer) {
    lexer->line++;
    lexer->column = 1;

    if (lexer->line_count == lexer->line_capacity) {
        lexer->line_capacity *= 2;
        lexer->line_starts = realloc(lexer->line_starts, lexer->line_capacity * sizeof(size_t));
    }

    lexer->line_starts[lexer->line_count++] = lexer->position;
}

static char lexer_peek_offset(const struct lexer *lexer, size_t offset)
{
    for (size_t i = 0; i < offset; i++) {
        if (lexer->source[i] == '\0') {
            return '\0';
        }
    }
    return lexer->source[lexer->position + offset];
}

static char lexer_advance(struct lexer *lexer)
{
    const char c = lexer->source[lexer->position];
    lexer->position++;
    lexer->column++;
    return c;
}

static bool lexer_match(struct lexer *lexer, const char expected)
{
    if (lexer_is_at_end(lexer)) {
        return false;
    }
    if (lexer->source[lexer->position] != expected) {
        return false;
    }
    lexer->position++;
    lexer->column++;
    return true;
}

static void lexer_add_token(struct lexer *lexer, const enum token_type type,
                            const char *lexeme_start, const size_t length,
                            const size_t line, const size_t column)
{
    token_t token;
    token.type   = type;
    token.lexeme = strndup(lexeme_start, length);
    token.length = length;
    token.line   = line;
    token.column = column;

    token_t_vector_push_back(&lexer->tokens, token);
}

static error_context_t lexer_make_context(const struct lexer *lexer) {
    error_context_t ctx;
    ctx.module = "Lexer";
    ctx.file = lexer->filename;
    ctx.line = (int)lexer->line;
    ctx.column = (int)lexer->column;

    ctx.source_line = lexer_get_current_line(lexer);

    return ctx;
}

static bool is_alpha_or_underscore(const char c)
{
    return (isalpha((unsigned char)c) || c == '_');
}

static bool is_alphanumeric_or_underscore(const char c)
{
    return (isalnum((unsigned char)c) || c == '_');
}

static bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static enum token_type lexer_identifier_type(const char *start, const size_t length)
{
#define KW_MATCH(str, ttype)                                        \
    if (length == (sizeof(str) - 1) &&                               \
        strncmp(start, str, (sizeof(str) - 1)) == 0) {              \
        return ttype;                                               \
    }

    KW_MATCH("function",  TOKEN_FUNCTION)
    KW_MATCH("fn",  TOKEN_FUNCTION)
    KW_MATCH("return",    TOKEN_RETURN)
    KW_MATCH("struct",    TOKEN_STRUCT)
    KW_MATCH("typedef",   TOKEN_TYPEDEF)
    KW_MATCH("if",        TOKEN_IF)
    KW_MATCH("else",      TOKEN_ELSE)
    KW_MATCH("while",     TOKEN_WHILE)
    KW_MATCH("switch",    TOKEN_SWITCH)
    KW_MATCH("case",      TOKEN_CASE)
    KW_MATCH("default",   TOKEN_DEFAULT)
    KW_MATCH("break",     TOKEN_BREAK)
    KW_MATCH("continue",  TOKEN_CONTINUE)
    KW_MATCH("extern",    TOKEN_EXTERN)
    KW_MATCH("void",      TOKEN_VOID)
    KW_MATCH("byte",      TOKEN_BYTE)
    KW_MATCH("word",      TOKEN_WORD)
    KW_MATCH("dword",     TOKEN_DWORD)
    KW_MATCH("qword",     TOKEN_QWORD)
    KW_MATCH("sbyte",     TOKEN_SBYTE)
    KW_MATCH("sword",     TOKEN_SWORD)
    KW_MATCH("sdword",    TOKEN_SDWORD)
    KW_MATCH("sqword",    TOKEN_SQWORD)

#undef KW_MATCH
    return TOKEN_IDENTIFIER;
}

static void lexer_scan_identifier(struct lexer *lexer)
{
    const size_t start_pos  = lexer->position - 1;
    const size_t start_line = lexer->line;
    const size_t start_col  = lexer->column - 1;

    while (is_alphanumeric_or_underscore(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }

    const size_t length = lexer->position - start_pos;
    const enum token_type ttype =
        lexer_identifier_type(&lexer->source[start_pos], length);

    lexer_add_token(lexer, ttype, &lexer->source[start_pos],
                    length, start_line, start_col);
}

static void lexer_scan_number(struct lexer *lexer)
{
    const size_t start_pos  = lexer->position - 1;
    const size_t start_line = lexer->line;
    const size_t start_col  = lexer->column - 1;

    if (lexer_peek(lexer) == 'x' || lexer_peek(lexer) == 'X') {
        lexer_advance(lexer); /* skip 'x'/'X' */
        while (isxdigit((unsigned char)lexer_peek(lexer))) {
            lexer_advance(lexer);
        }
        const size_t length = lexer->position - start_pos;
        lexer_add_token(lexer, TOKEN_HEXDECIMAL,
                        &lexer->source[start_pos],
                        length, start_line, start_col);
        return;
    }
    else if (lexer_peek(lexer) == 'b' || lexer_peek(lexer) == 'B') {
        lexer_advance(lexer); /* skip 'b'/'B' */
        while (lexer_peek(lexer) == '0' || lexer_peek(lexer) == '1') {
            lexer_advance(lexer);
        }
        const size_t length = lexer->position - start_pos;
        lexer_add_token(lexer, TOKEN_BINARY,
                        &lexer->source[start_pos],
                        length, start_line, start_col);
        return;
    }

    while (isdigit((unsigned char)lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    const size_t length = lexer->position - start_pos;
    lexer_add_token(lexer, TOKEN_DECIMAL,
                    &lexer->source[start_pos],
                    length, start_line, start_col);
}

static void lexer_scan_string(struct lexer *lexer)
{
    const size_t start_pos  = lexer->position - 1;
    const size_t start_line = lexer->line;
    const size_t start_col  = lexer->column - 1;

    while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '"') {
        if (lexer_peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 0;
        }
        lexer_advance(lexer);
    }

    if (lexer_is_at_end(lexer)) {
        const error_context_t ctx = lexer_make_context(lexer);
        compiler_error(&ctx,
            "Unterminated string literal starting at line %zu, column %zu",
            (size_t)start_line, (size_t)start_col);
    } else {
        lexer_advance(lexer);
    }

    const size_t length = lexer->position - start_pos;
    lexer_add_token(lexer, TOKEN_STRING_LITERAL,
                    &lexer->source[start_pos],
                    length, start_line, start_col);
}

static void lexer_scan_char_literal(struct lexer *lexer)
{
    const size_t start_pos  = lexer->position - 1;
    const size_t start_line = lexer->line;
    const size_t start_col  = lexer->column - 1;

    while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '\'') {
        if (lexer_peek(lexer) == '\n') {
            const error_context_t ctx = lexer_make_context(lexer);
            compiler_error(&ctx,
                "Unterminated character literal (newline encountered)");
            break;
        }
        lexer_advance(lexer);
    }

    if (!lexer_is_at_end(lexer)) {
        lexer_advance(lexer);
    } else {
        const error_context_t ctx = lexer_make_context(lexer);
        compiler_error(&ctx, "Unterminated character literal at EOF.");
    }

    const size_t length = lexer->position - start_pos;
    lexer_add_token(lexer, TOKEN_CHARACTER_LITERAL,
                    &lexer->source[start_pos],
                    length, start_line, start_col);
}

static void lexer_skip_whitespace_and_comments(struct lexer *lexer) {
    while (true) {
        const char c = lexer_peek(lexer);

        if (is_whitespace(c)) {
            if (c == '\n') {
                lexer_track_newline(lexer);
            }
            lexer_advance(lexer);
        }
        else if (c == '/' && lexer_peek_next(lexer) == '/') {
            while (lexer_peek(lexer) != '\n' && !lexer_is_at_end(lexer)) {
                lexer_advance(lexer);
            }
        }
        else if (c == '/' && lexer_peek_next(lexer) == '*') {
            lexer_advance(lexer);
            lexer_advance(lexer);
            while (!lexer_is_at_end(lexer)) {
                if (lexer_peek(lexer) == '*' &&
                    lexer_peek_next(lexer) == '/') {
                    lexer_advance(lexer);
                    lexer_advance(lexer);
                    break;
                }
                if (lexer_peek(lexer) == '\n') {
                    lexer_track_newline(lexer);
                }
                lexer_advance(lexer);
            }
        }
        else {
            break;
        }
    }
}

static void lexer_scan_all(struct lexer *lexer)
{
    while (!lexer_is_at_end(lexer)) {
        lexer_skip_whitespace_and_comments(lexer);

        if (lexer_is_at_end(lexer)) break;

        const size_t start_line = lexer->line;
        const size_t start_col  = lexer->column;
        char c = lexer_advance(lexer);

        if (is_alpha_or_underscore(c)) {
            lexer_scan_identifier(lexer);
            continue;
        }

        if (isdigit((unsigned char)c)) {
            lexer_scan_number(lexer);
            continue;
        }

        switch (c) {
            case '(':
                lexer_add_token(lexer, TOKEN_LEFT_PAREN, &c, 1, start_line, start_col);
                break;
            case ')':
                lexer_add_token(lexer, TOKEN_RIGHT_PAREN, &c, 1, start_line, start_col);
                break;
            case '{':
                lexer_add_token(lexer, TOKEN_LEFT_BRACE, &c, 1, start_line, start_col);
                break;
            case '}':
                lexer_add_token(lexer, TOKEN_RIGHT_BRACE, &c, 1, start_line, start_col);
                break;
            case '[':
                lexer_add_token(lexer, TOKEN_LEFT_BRACKET, &c, 1, start_line, start_col);
                break;
            case ']':
                lexer_add_token(lexer, TOKEN_RIGHT_BRACKET, &c, 1, start_line, start_col);
                break;
            case ';':
                lexer_add_token(lexer, TOKEN_SEMICOLON, &c, 1, start_line, start_col);
                break;
            case ':':
                lexer_add_token(lexer, TOKEN_COLON, &c, 1, start_line, start_col);
                break;
            case ',':
                lexer_add_token(lexer, TOKEN_COMMA, &c, 1, start_line, start_col);
                break;
            case '.':
                lexer_add_token(lexer, TOKEN_PERIOD, &c, 1, start_line, start_col);
                break;
            case '"':
                lexer_scan_string(lexer);
                break;
            case '\'':
                lexer_scan_char_literal(lexer);
                break;
            case '=':
                if (lexer_match(lexer, '=')) {
                    lexer_add_token(lexer, TOKEN_EQUAL,
                                    &lexer->source[lexer->position - 2],
                                    2, start_line, start_col);
                } else {
                    lexer_add_token(lexer, TOKEN_ASSIGN,
                                    &lexer->source[lexer->position - 1],
                                    1, start_line, start_col);
                }
                break;
            case '!':
                if (lexer_match(lexer, '=')) {
                    lexer_add_token(lexer, TOKEN_NOT_EQUAL,
                                    &lexer->source[lexer->position - 2],
                                    2, start_line, start_col);
                } else {
                    lexer_add_token(lexer, TOKEN_EXCLAMATION,
                                    &lexer->source[lexer->position - 1],
                                    1, start_line, start_col);
                }
                break;
            case '<':
                if (lexer_match(lexer, '=')) {
                    lexer_add_token(lexer, TOKEN_LESS_EQUAL,
                                    &lexer->source[lexer->position - 2],
                                    2, start_line, start_col);
                } else {
                    lexer_add_token(lexer, TOKEN_LESS,
                                    &lexer->source[lexer->position - 1],
                                    1, start_line, start_col);
                }
                break;
            case '>':
                if (lexer_match(lexer, '=')) {
                    lexer_add_token(lexer, TOKEN_GREATER_EQUAL,
                                    &lexer->source[lexer->position - 2],
                                    2, start_line, start_col);
                } else {
                    lexer_add_token(lexer, TOKEN_GREATER,
                                    &lexer->source[lexer->position - 1],
                                    1, start_line, start_col);
                }
                break;
            case '+':
                if (lexer_match(lexer, '+')) {
                    lexer_add_token(lexer, TOKEN_INCREMENT,
                                    &lexer->source[lexer->position - 2],
                                    2, start_line, start_col);
                } else {
                    lexer_add_token(lexer, TOKEN_PLUS,
                                    &lexer->source[lexer->position - 1],
                                    1, start_line, start_col);
                }
                break;
            case '-':
                if (lexer_match(lexer, '-')) {
                    lexer_add_token(lexer, TOKEN_DECREMENT,
                                    &lexer->source[lexer->position - 2],
                                    2, start_line, start_col);
                } else {
                    lexer_add_token(lexer, TOKEN_MINUS,
                                    &lexer->source[lexer->position - 1],
                                    1, start_line, start_col);
                }
                break;
            case '*':
                lexer_add_token(lexer, TOKEN_STAR, &c, 1, start_line, start_col);
                break;
            case '&':
                lexer_add_token(lexer, TOKEN_AMPERSAND, &c, 1, start_line, start_col);
                break;
            case '/':
                lexer_add_token(lexer, TOKEN_SLASH, &c, 1, start_line, start_col);
                break;
            case '|':
                lexer_add_token(lexer, TOKEN_PIPE, &c, 1, start_line, start_col);
                break;
            default: {
                error_context_t ctx = lexer_make_context(lexer);
                compiler_error(&ctx, "Unexpected character '%c' at line %zu, column %zu.",
                               c, (size_t)start_line, (size_t)start_col);
                break;
            }
        }
    }

    lexer_add_token(lexer, TOKEN_EOF, &lexer->source[lexer->position],
                    0, lexer->line, lexer->column);
}

void lexer_init(struct lexer *lexer, const char *source, const char* filename)
{
    lexer->source   = source;
    lexer->filename = filename;
    lexer->position = 0;
    lexer->line     = 1;
    lexer->column   = 1;
    token_t_vector_init(&lexer->tokens);

    lexer->line_count = 1;
    lexer->line_capacity = 16;
    lexer->line_starts = malloc(lexer->line_capacity * sizeof(size_t));
    lexer->line_starts[0] = 0;

    lexer_scan_all(lexer);
}

void lexer_destroy(struct lexer *lexer) {
    SAFE_FREE(lexer->line_starts);
    for (size_t i = 0; i < token_t_vector_size(&lexer->tokens); i++) {
        token_t *token = token_t_vector_at(&lexer->tokens, i);
        SAFE_FREE(token->lexeme);
    }
    token_t_vector_destroy(&lexer->tokens);
}

void lexer_add_tokens_to_vector(token_t_vector_t* origin, token_t_vector_t* tokens)
{
    for (size_t i = 0; i < token_t_vector_size(tokens); i++) {
        token_t token = *token_t_vector_at(tokens, i);
        token.lexeme = strdup(token.lexeme);
        token_t_vector_push_back(origin, token);
    }
}
