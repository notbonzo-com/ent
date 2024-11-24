//
// Created by notbonzo on 11/20/24.
//

#ifndef PARSER_HH
#define PARSER_HH

#include "Error.hh"
#include "Lexer.hh"
#include "AST.icc"
#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <set>

namespace ent {
    class parser_error : public error {
    public:
        parser_error(const std::string_view msg) : error(msg) {}
    };
    class parser_out_of_range final : public parser_error {
    public:
        parser_out_of_range(const std::string_view msg) : parser_error(msg) {}
    };
    class parser_expected_error final : public parser_error {
    public:
        explicit parser_expected_error(const std::string_view msg) : parser_error(msg) {}
    };
    class parser {
    public:
        explicit parser(std::vector<lexer::token>& tokens);

        ast::base_node_ptr parse();
        std::unordered_map<std::string_view, std::string_view> get_typedefs();
        std::unordered_map<std::string_view, std::vector<std::pair<std::string_view, std::string_view>>> get_structs();
    private:
        static int get_precedence(lexer::token::TOKEN_TYPE type);
        const lexer::token& consume();
        [[nodiscard]] const lexer::token& peak(int offset = 0) const;
        [[nodiscard]] const lexer::token& previous() const;
        void expect(lexer::token::TOKEN_TYPE expected, std::string_view str);
        [[nodiscard]] bool check(lexer::token::TOKEN_TYPE expected) const;

        [[nodiscard]] std::string resolve_typedef(std::string_view type) const;
        [[nodiscard]] bool is_type(std::string_view name) const;
        [[nodiscard]] bool is_struct_member(std::string_view struct_name, std::string_view member_name) const;
        std::string handle_type();

        ast::base_node_ptr parse_function();
        ast::base_node_ptr parse_extern();
        ast::base_node_ptr parse_global_variable_declaration(bool externV = false);
        ast::base_node_ptr parse_global_variable_declaration_assign();
        ast::base_node_ptr parse_block();
        ast::base_node_ptr parse_variable_declaration();
        ast::base_node_ptr parse_variable_declaration_assign();
        ast::base_node_ptr parse_while();
        ast::base_node_ptr parse_if();
        ast::base_node_ptr parse_switch();
        ast::base_node_ptr parse_function_call();
        ast::base_node_ptr parse_case();
        ast::base_node_ptr parse_expression();
        ast::base_node_ptr parse_literal();

        struct _scope {
            int operator++(int);
            int operator--(int);
            void add(std::string_view name);
            [[nodiscard]] bool is_declared(std::string_view name) const;
            std::stack<std::set<std::string_view>> m_scoped_stack;
        } m_scope;
        const std::vector<lexer::token>& m_tokens;
        std::unordered_map<std::string_view, std::string_view> m_typedefs;
        std::unordered_map<std::string, std::vector<std::string>> m_structs;
        std::vector<std::string_view> m_types;
        std::vector<std::string_view> m_function;
        std::vector<std::string_view> m_prototypes;
        size_t m_current = 0;
    };
} // ent

#endif //PARSER_HH
