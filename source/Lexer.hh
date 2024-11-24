//
// Created by notbonzo on 11/19/24.
//

#ifndef LEXER_HH
#define LEXER_HH

#include "Error.hh"
#include <string_view>
#include <string>
#include <vector>

namespace ent {
    class lexer_error : public error {
    public:
        explicit lexer_error(const std::string_view msg) : error(msg) {}
    };
    class lexer_out_of_range final : public lexer_error {
    public:
        explicit lexer_out_of_range(const size_t index, const size_t limit) : lexer_error(std::format("Out of range access to {}, limit is {}\n", index, limit)) {}
    };
    class lexer_expected_error final : public lexer_error {
    public:
        explicit lexer_expected_error(const std::string_view expected, const std::string_view got) : lexer_error(std::format("Expected {}, but got {}!\n", expected, got)) {}
    };
    class lexer {
    public:
        struct token {
            enum class TOKEN_TYPE {
                Identifier,
                Function, Return, Struct, Typedef, If, Else, While, Switch, Case, Default, Break, Continue, Extern,
                Void, Byte, Word, DWord, QWord, SByte, SWord, SDWord, SQWord,
                Decimal, Hexadecimal, Binary,
                StringLiteral,
                CharacterLiteral,
                LeftParen, RightParen,      // ( )
                LeftBrace, RightBrace,      // { }
                LeftBracket, RightBracket,  // []
                Semicolon,
                Colon,
                Comma,
                Period,
                Assign,
                Equal,
                NotEqual,
                Less,
                LessEqual,
                Greater,
                GreaterEqual,
                Plus, Minus, Increment, Decrement,
                Star, Ampersand,
                Slash,
                Pipe,
                Exclamation,
                EOFToken
            };
            TOKEN_TYPE type;
            std::string value;
            int line, column;
            token(const TOKEN_TYPE type, const std::string_view value, const int line, const int column) : type(type), value(value), line(line), column(column) {}
            token(const TOKEN_TYPE type, const int line, const int column) : type(type), line(line), column(column) {}

            [[nodiscard]] std::string_view to_string() const;
            [[nodiscard]] wchar_t to_symbol() const;

            bool operator==(const token& other) const;
            bool operator!=(const token& other) const;
        };
        explicit lexer(std::string_view preprocessed_file);
        std::vector<token>& get_tokens();
    private:
        char next();
        [[nodiscard]] char previous() const;
        [[nodiscard]] char peak(size_t index = 0) const;
        void match(char expected);
        void add_token(token::TOKEN_TYPE type, std::string_view value = std::string_view());

        void handle_identifier();
        [[nodiscard]] bool handle_keyword();
        void handle_number();
        void handle_string_literal();
        void handle_character_literal();
        void skip_line_comment();
        void skip_block_comment();
        void skip_whitespace();
        void handle_slash();

        std::string_view m_source;
        std::vector<token> m_tokens;
        size_t m_current = 0;
        size_t m_start = 0;
        int m_line = 0;
        int m_column = 0;
    };
} // ent

#endif //LEXER_HH
