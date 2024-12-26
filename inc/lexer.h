#ifndef LEXER_H
#define LEXER_H

#include <vector.h>
#include <stddef.h>
#include <stdbool.h>

enum token_type {
    TOKEN_IDENTIFIER,
    TOKEN_FUNCTION,
    TOKEN_RETURN,
    TOKEN_STRUCT,
    TOKEN_TYPEDEF,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_EXTERN,
    TOKEN_VOID,
    TOKEN_BYTE,
    TOKEN_WORD,
    TOKEN_DWORD,
    TOKEN_QWORD,
    TOKEN_SBYTE,
    TOKEN_SWORD,
    TOKEN_SDWORD,
    TOKEN_SQWORD,
    TOKEN_DECIMAL,
    TOKEN_HEXDECIMAL,
    TOKEN_BINARY,
    TOKEN_STRING_LITERAL,
    TOKEN_NUMBER,
    TOKEN_CHARACTER_LITERAL,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_PERIOD,
    TOKEN_ASSIGN,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_STAR,
    TOKEN_AMPERSAND,
    TOKEN_SLASH,
    TOKEN_PIPE,
    TOKEN_EXCLAMATION,
    TOKEN_EOF
};

typedef struct {
    enum token_type type;
    char *lexeme;
    size_t length;
    size_t line;
    size_t column;
} token_t;

DECLARE_VECTOR_TYPE(token_t)

struct lexer {
    const char* filename;
    const char *source;
    size_t position;
    size_t line;
    size_t column;
    token_t_vector_t tokens;

    size_t *line_starts;
    size_t line_count;
    size_t line_capacity;
};

void lexer_init(struct lexer *lexer, const char *source, const char* filename);
void lexer_destroy(struct lexer *lexer);

#endif /* LEXER_H */
