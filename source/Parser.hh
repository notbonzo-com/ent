#ifndef PARSER_HH
#define PARSER_HH

#include "Lexer.hh"
#include "AST.icc"
#include "Error.hh"
#include <stdexcept>
#include <string_view>
#include <string>
#include <optional>
#include <format>

namespace ent {

    class parser_error final : public error {
    public:
        parser_error(const std::string& msg, int line, int col)
            : error(std::format("{} at line {}, column {}", msg, line, col)) {}
    };

    class parser {
    public:
        explicit parser(std::vector<lexer::token> tokens)
            : m_tokens(std::move(tokens)) {}

        ast::base_node_ptr parse_program();

    private:
        [[nodiscard]] const lexer::token& peek(size_t offset = 0) const;
        [[nodiscard]] const lexer::token& current() const;
        [[nodiscard]] const lexer::token& previous() const;
        bool match(lexer::token::TOKEN_TYPE type);
        [[nodiscard]] bool check(lexer::token::TOKEN_TYPE type) const;
        void advance();
        void consume(lexer::token::TOKEN_TYPE type, std::string_view message);
        [[noreturn]] static void error(const lexer::token& tok, std::string_view message);
        [[nodiscard]] bool is_at_end() const;

        ast::base_node_ptr parse_top_level_decl();
        ast::base_node_ptr parse_function(bool is_extern = false);
        ast::base_node_ptr parse_function_prototype(bool is_extern = false);
        ast::base_node_ptr parse_global_variable(bool is_extern);
        variable_type parse_type();

        static bool is_type_keyword(const lexer::token& tok);

        // Statements
        ast::base_node_ptr parse_statement();
        ast::base_node_ptr parse_if_statement();
        ast::base_node_ptr parse_while_statement();
        ast::base_node_ptr parse_switch_statement();
        ast::base_node_ptr parse_case_statement();
        ast::base_node_ptr parse_return_statement();
        ast::base_node_ptr parse_break_statement();
        ast::base_node_ptr parse_continue_statement();
        ast::base_node_ptr parse_block();

        // Declarations
        ast::base_node_ptr parse_variable_declaration(bool allow_extern = false);

        // Expressions
        ast::base_node_ptr parse_expression();
        ast::base_node_ptr parse_logical_or_expr();
        ast::base_node_ptr parse_logical_and_expr();
        ast::base_node_ptr parse_equality_expr();
        ast::base_node_ptr parse_relational_expr();
        ast::base_node_ptr parse_additive_expr();
        ast::base_node_ptr parse_multiplicative_expr();
        ast::base_node_ptr parse_unary_expr();
        ast::base_node_ptr parse_primary_expr();
        ast::base_node_ptr parse_function_call_or_variable();
        ast::base_node_ptr parse_assignment_expr();

        static ast::EXPRESSION_NODE_OP token_to_expression_op(lexer::token::TOKEN_TYPE type);

        static bool is_unary_operator(const lexer::token& tok);
        static bool is_binary_operator(const lexer::token& tok);
        ast::base_node_ptr parse_postfix_operators(ast::base_node_ptr expr);

        std::vector<lexer::token> m_tokens;
        size_t m_current = 0;
    };

} // namespace ent

#endif