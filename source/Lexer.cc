//
// Created by notbonzo on 11/19/24.
//

#include "Lexer.hh"
#include <map>
#include <cctype>

namespace ent {

    static const std::map<std::string_view, lexer::token::TOKEN_TYPE> keywords = {
        {"fn", lexer::token::TOKEN_TYPE::Function},
        {"return", lexer::token::TOKEN_TYPE::Return},
        {"extern", lexer::token::TOKEN_TYPE::Extern},
        {"void", lexer::token::TOKEN_TYPE::Void},
        {"typedef", lexer::token::TOKEN_TYPE::Typedef},
        {"struct", lexer::token::TOKEN_TYPE::Struct},
        {"if", lexer::token::TOKEN_TYPE::If},
        {"else", lexer::token::TOKEN_TYPE::Else},
        {"while", lexer::token::TOKEN_TYPE::While},
        {"switch", lexer::token::TOKEN_TYPE::Switch},
        {"case", lexer::token::TOKEN_TYPE::Case},
        {"default", lexer::token::TOKEN_TYPE::Default},
        {"break", lexer::token::TOKEN_TYPE::Break},
        {"continue", lexer::token::TOKEN_TYPE::Continue},
        {"sbyte", lexer::token::TOKEN_TYPE::SByte},
        {"sword", lexer::token::TOKEN_TYPE::SWord},
        {"sdword", lexer::token::TOKEN_TYPE::SDWord},
        {"sqword", lexer::token::TOKEN_TYPE::SQWord},
        {"byte", lexer::token::TOKEN_TYPE::Byte},
        {"word", lexer::token::TOKEN_TYPE::Word},
        {"dword", lexer::token::TOKEN_TYPE::DWord},
        {"qword", lexer::token::TOKEN_TYPE::QWord},
    };

    [[nodiscard]] std::string_view lexer::token::to_string() const {
        switch (type) {
            case TOKEN_TYPE::Function: return "Function";
            case TOKEN_TYPE::Return: return "Return";
            case TOKEN_TYPE::Extern: return "Extern";
            case TOKEN_TYPE::Void: return "Void";
            case TOKEN_TYPE::Typedef: return "Typedef";
            case TOKEN_TYPE::Struct: return "Struct";
            case TOKEN_TYPE::If: return "If";
            case TOKEN_TYPE::Else: return "Else";
            case TOKEN_TYPE::While: return "While";
            case TOKEN_TYPE::Switch: return "Switch";
            case TOKEN_TYPE::Case: return "Case";
            case TOKEN_TYPE::Default: return "Default";
            case TOKEN_TYPE::Break: return "Break";
            case TOKEN_TYPE::Continue: return "Continue";
            case TOKEN_TYPE::SByte: return "SByte";
            case TOKEN_TYPE::SWord: return "SWord";
            case TOKEN_TYPE::SDWord: return "SDWord";
            case TOKEN_TYPE::QWord: return "QWord";
            case TOKEN_TYPE::Byte: return "Byte";
            case TOKEN_TYPE::Word: return "Word";
            case TOKEN_TYPE::DWord: return "DWord";
            case TOKEN_TYPE::Identifier: return "identifier";
            case TOKEN_TYPE::Decimal: return "decimal_number";
            case TOKEN_TYPE::Binary: return "binary_number";
            case TOKEN_TYPE::Hexadecimal: return "hexadecimal_number";
            case TOKEN_TYPE::StringLiteral: return "string_literal";
            case TOKEN_TYPE::CharacterLiteral: return "character_literal";
            case TOKEN_TYPE::LeftParen: return "left_paren";
            case TOKEN_TYPE::RightParen: return "right_paren";
            case TOKEN_TYPE::LeftBrace: return "left_brace";
            case TOKEN_TYPE::RightBrace: return "right_brace";
            case TOKEN_TYPE::LeftBracket: return "left_bracket";
            case TOKEN_TYPE::Colon: return "colon";
            case TOKEN_TYPE::RightBracket: return "right_bracket";
            case TOKEN_TYPE::Semicolon: return "semicolon";
            case TOKEN_TYPE::Comma: return "comma";
            case TOKEN_TYPE::Period: return "period";
            case TOKEN_TYPE::Assign: return "assign";
            case TOKEN_TYPE::Equal: return "equal";
            case TOKEN_TYPE::NotEqual: return "not_equal";
            case TOKEN_TYPE::Less: return "less";
            case TOKEN_TYPE::LessEqual: return "less_equal";
            case TOKEN_TYPE::Greater: return "greater";
            case TOKEN_TYPE::GreaterEqual: return "greater_equal";
            case TOKEN_TYPE::Plus: return "plus";
            case TOKEN_TYPE::Minus: return "minus";
            case TOKEN_TYPE::Increment: return "increment";
            case TOKEN_TYPE::Decrement: return "decrement";
            case TOKEN_TYPE::Star: return "star";
            case TOKEN_TYPE::Ampersand: return "ampersand";
            case TOKEN_TYPE::Slash: return "slash";
            case TOKEN_TYPE::Pipe: return "pipe";
            case TOKEN_TYPE::Exclamation: return "exclamation";
            case TOKEN_TYPE::EOFToken: return "eof_token";
            default: return "<unknown>";
        }
    }

    [[nodiscard]] wchar_t lexer::token::to_symbol() const {
        switch (type) {
            case TOKEN_TYPE::LeftParen: return '(';
            case TOKEN_TYPE::RightParen: return ')';
            case TOKEN_TYPE::LeftBrace: return '{';
            case TOKEN_TYPE::RightBrace: return '}';
            case TOKEN_TYPE::LeftBracket: return '[';
            case TOKEN_TYPE::RightBracket: return ']';
            case TOKEN_TYPE::Semicolon: return ';';
            case TOKEN_TYPE::Comma: return ',';
            case TOKEN_TYPE::Period: return '.';
            case TOKEN_TYPE::Assign: return '=';
            case TOKEN_TYPE::Colon: return ':';
            case TOKEN_TYPE::Equal: return L'=';
            case TOKEN_TYPE::NotEqual: return L'!';
            case TOKEN_TYPE::Less: return '<';
            case TOKEN_TYPE::LessEqual: return L'<';
            case TOKEN_TYPE::Greater: return '>';
            case TOKEN_TYPE::GreaterEqual: return L'>';
            case TOKEN_TYPE::Plus: return '+';
            case TOKEN_TYPE::Minus: return '-';
            case TOKEN_TYPE::Increment: return '+';
            case TOKEN_TYPE::Decrement: return '-';
            case TOKEN_TYPE::Star: return '*';
            case TOKEN_TYPE::Ampersand: return '&';
            case TOKEN_TYPE::Slash: return '/';
            case TOKEN_TYPE::Pipe: return '|';
            case TOKEN_TYPE::Exclamation: return '!';
            case TOKEN_TYPE::EOFToken: return '\n';
            default: return '\0';
        }
    }

    bool lexer::token::operator==(const token& other) const {
        return type == other.type;
    }

    bool lexer::token::operator!=(const token& other) const {
        return type != other.type;
    }

    char lexer::next() {
        if (m_current >= m_source.size()) {
            throw lexer_out_of_range(m_current, m_source.size());
        }
        return m_source[m_current++];
    }

    [[nodiscard]] char lexer::previous() const {
        if (m_current <= 0) {
            throw lexer_out_of_range(m_current, m_source.size());
        }
        return m_source[m_current - 1];
    }

    [[nodiscard]] char lexer::peak(const size_t index) const {
        if (m_current + index >= m_source.size()) {
            throw lexer_out_of_range(m_current + index, m_source.size());
        }
        return m_source[m_current + index];
    }

    void lexer::match(const char expected) {
        if (m_current >= m_source.size()) {
            throw lexer_out_of_range(m_current, m_source.size());
        }
        if (m_source[m_current] != expected) {
            throw lexer_expected_error(std::string(1, expected), std::string(1, m_source[m_current]));
        }
        ++m_current;
    }

    void lexer::add_token(const token::TOKEN_TYPE type, const std::string_view value) {
        if (value.empty()) {
            m_tokens.emplace_back(type, m_line, m_column);
        } else {
            m_tokens.emplace_back(type, value, m_line, m_column);
        }
    }

    lexer::lexer(const std::string_view preprocessed_file) : m_source(preprocessed_file) {
        m_tokens.reserve(m_source.size() / 4);
        while (m_current < m_source.size()) {
            skip_whitespace();
            m_start = m_current;
            if (m_current >= m_source.size()) break;

            const char c = next();
            switch (c) {
                case '(': add_token(token::TOKEN_TYPE::LeftParen); break;
                case ')': add_token(token::TOKEN_TYPE::RightParen); break;
                case '{': add_token(token::TOKEN_TYPE::LeftBrace); break;
                case '}': add_token(token::TOKEN_TYPE::RightBrace); break;
                case '[': add_token(token::TOKEN_TYPE::LeftBracket); break;
                case ']': add_token(token::TOKEN_TYPE::RightBracket); break;
                case ',': add_token(token::TOKEN_TYPE::Comma); break;
                case '.': add_token(token::TOKEN_TYPE::Period); break;
                case ';': add_token(token::TOKEN_TYPE::Semicolon); break;
                case '&': add_token(token::TOKEN_TYPE::Ampersand); break;
                case '|': add_token(token::TOKEN_TYPE::Pipe); break;
                case '*': add_token(token::TOKEN_TYPE::Star); break;
                case ':': add_token(token::TOKEN_TYPE::Colon); break;
                case '/': handle_slash(); break;
                case '=':
                    if (peak() == '=') { next(); add_token(token::TOKEN_TYPE::Equal); }
                    else { add_token(token::TOKEN_TYPE::Assign); }
                    break;
                case '!':
                    if (peak() == '=') { next(); add_token(token::TOKEN_TYPE::NotEqual); }
                    else { add_token(token::TOKEN_TYPE::Exclamation); }
                    break;
                case '<':
                    if (peak() == '=') { next(); add_token(token::TOKEN_TYPE::LessEqual); }
                    else { add_token(token::TOKEN_TYPE::Less); }
                    break;
                case '>':
                    if (peak() == '=') { next(); add_token(token::TOKEN_TYPE::GreaterEqual); }
                    else { add_token(token::TOKEN_TYPE::Greater); }
                    break;
                case '+':
                    if (peak() == '+') { next(); add_token(token::TOKEN_TYPE::Increment); }
                    else { add_token(token::TOKEN_TYPE::Plus); }
                    break;
                case '-':
                    if (peak() == '-') { next(); add_token(token::TOKEN_TYPE::Decrement); }
                    else { add_token(token::TOKEN_TYPE::Minus); }
                    break;
                case '\'': handle_character_literal(); break;
                case '"': handle_string_literal(); break;
                default:
                    if (std::isdigit(c)) { handle_number(); }
                    else if (std::isalpha(c) || c == '_') {
                        if (!handle_keyword()) { handle_identifier(); }
                    } else {
                        throw lexer_expected_error("valid symbol or expression", std::string(1, c));
                    }
            }
        }
        add_token(token::TOKEN_TYPE::EOFToken);
    }

    std::vector<lexer::token>& lexer::get_tokens() {
        return m_tokens;
    }

    void lexer::handle_character_literal() {
        const char c = next();
        std::string value;
        if (c == '\\') {
            switch (next()) {
                case 'n': value = '\n'; break;
                case 't': value = '\t'; break;
                case '\\': value = '\\'; break;
                case '\'': value = '\''; break;
                default: throw lexer_expected_error("valid escape sequence", std::string(1, c));
            }
        } else {
            value = c;
        }
        match('\'');
        add_token(token::TOKEN_TYPE::CharacterLiteral, value);
    }

    bool lexer::handle_keyword() {
        while (std::isalnum(peak()) || peak() == '_') { next(); }
        const std::string_view text = m_source.substr(m_start, m_current - m_start);
        if (keywords.contains(text)) {
            add_token(keywords.at(text), text);
            return true;
        }
        return false;
    }

    void lexer::handle_identifier() {
        while (std::isalnum(peak()) || peak() == '_') { next(); }
        add_token(token::TOKEN_TYPE::Identifier, m_source.substr(m_start, m_current - m_start));
    }

    void lexer::handle_string_literal() {
        std::string value;
        while (peak() != '"' && m_current < m_source.size()) {
            if (peak() == '\\') {
                next();
                switch (next()) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case '\\': value += '\\'; break;
                    case '"': value += '"'; break;
                    default: throw lexer_expected_error("valid escape sequence", std::string(1, peak()));
                }
            } else {
                value += next();
            }
        }
        if (m_current >= m_source.size()) {
            throw lexer_out_of_range(m_current, m_source.size());
        }
        match('"');
        add_token(token::TOKEN_TYPE::StringLiteral, value);
    }

    void lexer::handle_slash() {
        if (peak() == '/') {
            skip_line_comment();
        } else if (peak() == '*') {
            next();
            skip_block_comment();
        } else {
            add_token(token::TOKEN_TYPE::Slash);
        }
    }

    void lexer::skip_line_comment() {
        while (peak() != '\n' && m_current < m_source.size()) { next(); }
        if (peak() == '\n') {
            next();
            m_line++;
            m_column = 1;
        }
    }

    void lexer::skip_block_comment() {
        while (!(peak() == '*' && peak(1) == '/') && m_current < m_source.size()) { next(); }
        if (m_current >= m_source.size()) {
            throw lexer_out_of_range(m_current, m_source.size());
        }
        match('*');
        match('/');
    }

    void lexer::skip_whitespace() {
        while (m_current < m_source.size()) {
            switch (peak()) {
                case ' ': case '\r': case '\t':
                    next();
                    m_column++;
                    break;
                case '\n':
                    next();
                    m_line++;
                    m_column = 1;
                    break;
                default:
                    return;
            }
        }
    }

    void lexer::handle_number() {
        std::string number;
        if (previous() == '0') {
            const char c = next();
            if (c == 'b') {
                while (std::isdigit(peak()) && (peak() == '0' || peak() == '1')) { number += next(); }
                add_token(token::TOKEN_TYPE::Binary, number);
                return;
            }
            if (c == 'x') {
                while (std::isxdigit(peak())) { number += next(); }
                add_token(token::TOKEN_TYPE::Hexadecimal, number);
                return;
            }
            if (!std::isalnum(c)) {
                add_token(token::TOKEN_TYPE::Decimal, "0");
                return;
            }

            throw lexer_expected_error("binary or hexadecimal number prefix", std::string(1, c));
        }
        while (std::isdigit(peak())) { number += next(); }
        add_token(token::TOKEN_TYPE::Decimal, number);
    }

} // namespace ent
