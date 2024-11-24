//
// Created by notbonzo on 11/20/24.
//

#include "Parser.hh"
#include <string>
#include <functional>

namespace ent {
    parser::parser(std::vector<lexer::token>& tokens) : m_tokens(tokens), m_types({
            "void", "byte", "word", "dword", "qword", "sbyte", "sword", "sdword", "sqword"
    }) {
        m_scope.m_scoped_stack.emplace(); // global stack
    }
    using TOKEN_TYPE = lexer::token::TOKEN_TYPE;
    int parser::get_precedence(const lexer::token::TOKEN_TYPE type) {
        switch (type) {
            case TOKEN_TYPE::Star:
            case TOKEN_TYPE::Slash: return 3; // Highest precedence
            case TOKEN_TYPE::Plus:
            case TOKEN_TYPE::Minus: return 2;
            case TOKEN_TYPE::Equal:
            case TOKEN_TYPE::NotEqual:
            case TOKEN_TYPE::Less:
            case TOKEN_TYPE::LessEqual:
            case TOKEN_TYPE::Greater:
            case TOKEN_TYPE::GreaterEqual: return 1; // Lowest precedence
            default: return 0;
        }
    }
    void parser::_scope::add(const std::string_view name) {
        if (m_scoped_stack.empty()) {
            throw parser_out_of_range("parser::_scope::add(): scope is empty");
        }
        m_scoped_stack.top().insert(name);
    }
    int parser::_scope::operator++(int) {
        m_scoped_stack.emplace();
        return 0;
    }
    int parser::_scope::operator--(int) {
        if (m_scoped_stack.empty()) {
            throw parser_out_of_range("parser::_scope::operator--: empty stack");
        }
        m_scoped_stack.pop();
        return 0;
    }
    bool parser::_scope::is_declared(const std::string_view name) const {
        std::stack<std::set<std::string_view>> tmp_stack = m_scoped_stack;
        while (!tmp_stack.empty()) {
            if (const std::set<std::string_view>& current_set = tmp_stack.top(); current_set.contains(name)) {
                return true;
            }
            tmp_stack.pop();
        }
        return false;
    }

    const lexer::token& parser::consume() {
        if (m_current >= m_tokens.size()) {
            throw parser_out_of_range("parser::consume(): token out of range");
        }
        return m_tokens[m_current++];
    }

    const lexer::token& parser::peak(const int offset) const {
        if (m_current + offset >= m_tokens.size()) {
            throw std::out_of_range("parser::peak(): token out of range");
        }
        return m_tokens[m_current + offset];
    }

    const lexer::token& parser::previous() const {
        [[unlikely]] if (m_current == 0) {
            throw std::out_of_range("parser::previous(): empty stack");
        }
        return m_tokens[m_current - 1];
    }

    void parser::expect(const lexer::token::TOKEN_TYPE expected, const std::string_view str) {
        if (!check(expected)) {
            throw parser_error(std::format(
                "parser::handle_type(): Unexpected token '{}' at line {}, column {}. {}.",
                peak().to_string(), peak().line, peak().column, str));

        }

        consume();
    }

    bool parser::check(const lexer::token::TOKEN_TYPE expected) const {
        if (m_current >= m_tokens.size()) {
            throw parser_out_of_range("parser::check(): token out of range");
        }
        return peak().type == expected;
    }

    std::string parser::resolve_typedef(const std::string_view type) const {
        auto current = std::string(type);
        while (true) {
            const auto it = m_typedefs.find(current);
            if (it == m_typedefs.end()) {
                break;
            }
            current = it->second;

            if (current == type) {
                throw std::runtime_error("parser::resolve_typedef(): cyclic typedef detected");
            }
        }
        return current;
    }

    bool parser::is_type(const std::string_view name) const {
        const auto last_non_star = name.find_last_not_of('*');
        const auto base_type = name.substr(0, last_non_star + 1);
        return std::ranges::find(m_types, base_type) != m_types.end();
    }

    bool parser::is_struct_member(const std::string_view struct_name, const std::string_view member_name) const {
        if (const auto it = m_structs.find(std::string(struct_name)); it != m_structs.end()) {
            const auto& members = it->second;
            return std::ranges::find(members, member_name) != members.end();
        }
        return false;
    }
    std::string parser::handle_type() {
        const std::string base_type = consume().value;
        if (!is_type(base_type)) {
            throw parser_error("parser::handle_type(): Expected a valid base type");
        }

        std::string full_type = base_type;
        while (check(TOKEN_TYPE::Star)) {
            consume();
            full_type += '*';
        }

        return full_type;
    }

    ast::base_node_ptr parser::parse() {
        std::vector<ast::base_node_ptr> statements;
        while (!check(TOKEN_TYPE::EOFToken)) {
            if (check(TOKEN_TYPE::Function)) {
                statements.push_back(parse_function());
            } else if (check(TOKEN_TYPE::Typedef)) { // TODO
            } else if (check(TOKEN_TYPE::Struct)) { // TODO
            } else if (check(TOKEN_TYPE::Extern)) {
                statements.push_back(parse_extern());
            } else if (is_type(peak().value)) {
                if (peak(2).type == TOKEN_TYPE::Semicolon) {
                    statements.push_back(parse_global_variable_declaration());
                } else if (peak(2).type == TOKEN_TYPE::Assign) {
                    statements.push_back(parse_global_variable_declaration_assign());
                }
                throw parser_expected_error("parser::parse(): Expected ';' or '=' after variable declaration");
            } else {
                std::println("Got {}!", peak().to_string());
                throw parser_expected_error("parser::parse(): Expected statement");
            }
        }
        return std::make_shared<ast::program_node>(std::move(statements));
    }

    ast::base_node_ptr parser::parse_function() {
        std::vector<ast::base_node_ptr> parameters;
        expect(TOKEN_TYPE::Function, "parser::parse_function(): Expected 'fn' keyword for function definition");
        const std::string_view name = consume().value;
        if (name.empty()) {
            throw parser_error("parser::parse_function(): Functions name is empty");
        }
        m_scope++;
        expect(TOKEN_TYPE::LeftParen, "parser::parse_function(): Expected '('");
        if (!check(TOKEN_TYPE::RightParen)) {
            const std::string_view type = handle_type();
            if (type.empty()) {
                throw parser_expected_error("parser::parse_function(): Expected ')'");
            }
            if (!is_type(type)) {
                std::println("Got type: '{}'", type);
                throw parser_expected_error("parser::parse_function(): Expected parameter type after '('");
            }
            const std::string_view parameter_name = consume().value;
            parameters.push_back(std::make_shared<ast::parameter_node>(parameter_name, type));
            m_scope.add(parameter_name);
            while (check(TOKEN_TYPE::Comma)) {
                expect(TOKEN_TYPE::Comma, "parser::parse_function(): Expected ','");
                const std::string_view loop_type = handle_type();
                if (loop_type.empty() || !is_type(loop_type)) {
                    throw parser_expected_error("parser::parse_function(): Expected type name");
                }
                std::string_view loop_parameter_name = consume().value;
                parameters.push_back(std::make_shared<ast::parameter_node>(loop_parameter_name, loop_type));
                m_scope.add(loop_parameter_name);
            }
        }
        expect(TOKEN_TYPE::RightParen, "parser::parse_function(): Expected ')'");
        expect(TOKEN_TYPE::Minus, "parser::parse_function(): Expected '->'");
        expect(TOKEN_TYPE::Greater, "parser::parse_function(): Expected '->'");

        const std::string_view return_type = handle_type();
        if (!is_type(return_type)) {
            throw parser_expected_error("parser::parse_function(): Expected return type");
        }
        if (check(TOKEN_TYPE::LeftBrace)) {
            expect(TOKEN_TYPE::LeftBrace, "parser::parse_function(): Expected '{'");
            const ast::base_node_ptr body = parse_block();
            expect(TOKEN_TYPE::RightBrace, "parser::parse_function(): Expected '}'");
            m_scope--;
            expect(TOKEN_TYPE::Semicolon, "parser::parse_function(): Expected ';'");

            m_prototypes.push_back(name);
            m_function.push_back(name);
            return std::make_shared<ast::function_node>(return_type, name, parameters, body);
        }
        expect(TOKEN_TYPE::Semicolon, "parser::parse_function(): Expected ';'");
        m_scope--; // discard the scope, as we are not a function
        if (std::ranges::find(m_prototypes, name) != m_prototypes.end()) { // TODO: Replace with a function
            throw parser_error("parser::parse_function(): Function prototype already defined.");
        }
        m_prototypes.push_back(name);
        return std::make_shared<ast::function_prototype_node>(return_type, name, parameters);
    }

    ast::base_node_ptr parser::parse_extern() {
        expect(TOKEN_TYPE::Extern, "parser::parse_extern(): Expected 'extern' keyword");

        if (check(TOKEN_TYPE::Function)) {
            ast::base_node_ptr function = parse_function();
            if (function->type() != ast::NODE_TYPE::FunctionPrototype) {
                throw parser_error("parser::parse_extern(): Function definition can not be extern");
            }
            return std::make_shared<ast::extern_node>(function);
        }
        if (is_type(peak().value)) {
            return std::make_shared<ast::extern_node>(parse_global_variable_declaration(true));
        }
        throw std::runtime_error("parser::parse_extern(): Expected declaration of extern entity");
    }

    ast::base_node_ptr parser::parse_global_variable_declaration(const bool externV) {
        const std::string_view type = handle_type();
        if (type.empty() || !is_type(type)) {
            throw parser_error("parser::parse_global_variable_declaration(): Expected variable type");
        }
        const std::string_view name = consume().value;
        if (name.empty()) {
            throw parser_error("parser::parse_global_variable_declaration(): Expected variable name");
        }
        if (m_scope.is_declared(name) && !externV) {
            throw parser_error("parser::parse_global_variable_declaration(): Variable already in use");
        }
        expect(TOKEN_TYPE::Semicolon, "parser::parse_global_variable_declaration(): Expected ';'");
        m_scope.add(name);
        return std::make_shared<ast::variable_declaration_node>(name, type);
    }

    ast::base_node_ptr parser::parse_global_variable_declaration_assign() {
        const std::string_view type = handle_type();
        if (type.empty() || !is_type(type)) {
            throw parser_error("parser::parse_global_variable_declaration_assign(): Expected variable type");
        }
        const std::string_view name = consume().value;
        if (name.empty()) {
            throw parser_error("parser::parse_global_variable_declaration_assign(): Expected variable name");
        }
        if (m_scope.is_declared(name)) {
            throw parser_error("parser::parse_global_variable_declaration_assign(): Variable already in use");
        }
        expect(TOKEN_TYPE::Assign, "parser::parse_global_variable_declaration_assign(): Expected '='");
        ast::base_node_ptr rhs = parse_expression();
        expect(TOKEN_TYPE::Semicolon, "parser::parse_global_variable_declaration_assign(): Expected ';'");
        m_scope.add(name);
        return std::make_shared<ast::variable_declaration_assign_node>(name, type, rhs);
    }

    ast::base_node_ptr parser::parse_block() {
        std::vector<ast::base_node_ptr> statements;
        m_scope++;
        while (!check(TOKEN_TYPE::RightBrace) && !check(TOKEN_TYPE::EOFToken)) {
            if (is_type(peak().value) &&
                peak(1).type == TOKEN_TYPE::Identifier) {
                if (peak(2).type == TOKEN_TYPE::Semicolon) {
                    statements.push_back(parse_variable_declaration());
                } else if (peak(1).type == TOKEN_TYPE::Assign) {
                    statements.push_back(parse_variable_declaration_assign());
                } else {
                    throw parser_error("parser::parse_block(): Expected '=' or ';' after variable declaration");
                }
            }

            else if (check(TOKEN_TYPE::While)) {
                statements.push_back(parse_while());
            }

            else if (check(TOKEN_TYPE::If)) {
                statements.push_back(parse_if());
            }

            else if (check(TOKEN_TYPE::Switch)) {
                statements.push_back(parse_switch());
            }

            else if (check(TOKEN_TYPE::Return)) {
                consume();
                ast::base_node_ptr expression = parse_expression();
                statements.push_back(std::make_shared<ast::return_node>(expression));
            }

            else if (check(TOKEN_TYPE::Break)) {
                consume();
                statements.push_back(std::make_shared<ast::break_node>());
            }

            else if (check(TOKEN_TYPE::Continue)) {
                consume();
                statements.push_back(std::make_shared<ast::continue_node>());
            }

            else if (check(TOKEN_TYPE::Identifier)) {
                if (m_scope.is_declared(peak().value)) {
                    if (peak(1).type == TOKEN_TYPE::Increment) {
                        statements.push_back(std::make_shared<ast::increment_node>(consume().value));
                        consume();
                        expect(TOKEN_TYPE::Semicolon, "parser::parse_block(): Expected ';' after post-increment");
                    } else if (peak(1).type == TOKEN_TYPE::Decrement) {
                        statements.push_back(std::make_shared<ast::decrement_node>(consume().value));
                        consume();
                        expect(TOKEN_TYPE::Semicolon, "parser::parse_block(): Expected ';' after post-decrement");
                    } else if (peak(1).type == TOKEN_TYPE::Assign) {
                        std::string_view name = consume().value;
                        if (name.empty()) {
                            throw parser_error("parser::parse_block(): Expected variable name");
                        }
                        expect(TOKEN_TYPE::Assign, "parser::parse_block(): Expected '=' after variable name");
                        ast::base_node_ptr rhs = parse_expression();
                        expect(TOKEN_TYPE::Semicolon, "parser::parse_block(): Expected ';' after assignment");
                        statements.push_back(std::make_shared<ast::assignment_node>(name, rhs));
                    } else if (peak(1).type == TOKEN_TYPE::LeftBracket) {
                        std::string_view name = consume().value;
                        expect(TOKEN_TYPE::LeftBracket, "parser::parse_block(): Expected '['");
                        ast::base_node_ptr index = parse_expression();
                        expect(TOKEN_TYPE::RightBracket, "parser::parse_block(): Expected ']'");
                        expect(TOKEN_TYPE::Assign, "parser::parse_block(): Expected '='");
                        ast::base_node_ptr rhs = parse_expression();
                        expect(TOKEN_TYPE::Semicolon, "parser::parse_block(): Expected ';' after assignment");
                        statements.push_back(std::make_shared<ast::index_assignment_node>(name, index, rhs));
                    } else if (peak(1).type == TOKEN_TYPE::Period) {
                        std::string_view base_name = consume().value;
                        if (base_name.empty()) {
                            throw parser_error("parser::parse_block(): Expected identifier before '.'");
                        }
                        ast::base_node_ptr base = std::make_shared<ast::variable_node>(base_name);

                        while (check(TOKEN_TYPE::Period)) {
                            consume();
                            std::string_view member_name = consume().value;

                            if (member_name.empty()) {
                                throw parser_error("parser::parse_block(): Expected member name after '.'");
                            }

                            if (check(TOKEN_TYPE::LeftParen)) {
                                consume();
                                std::vector<ast::base_node_ptr> args;

                                if (!check(TOKEN_TYPE::RightParen)) {
                                    do {
                                        args.push_back(parse_expression());
                                    } while (check(TOKEN_TYPE::Comma) && consume().type == TOKEN_TYPE::Comma);
                                }

                                expect(TOKEN_TYPE::RightParen, "parser::parse_block(): Expected ')' after function arguments");

                                // Transform into UFCS: `variable.method(args...)` -> `method(variable, args...)`
                                args.insert(args.begin(), base);
                                base = std::make_shared<ast::element_call_node>(member_name, args);
                            } else {
                                base = std::make_shared<ast::member_invoke_node>(base, member_name);
                            }
                        }

                        expect(TOKEN_TYPE::Semicolon, "parser::parse_block(): Expected ';' after member access or UFCS");
                        statements.push_back(base);
                    }
                } else if (std::ranges::find(m_function, peak().value) != m_function.end()) { // TODO: Make into a function
                    statements.push_back(parse_function_call());
                } else {
                    throw parser_error("parser::parse_block(): undefined variable or function name");
                }
            } else if (check(TOKEN_TYPE::Star)) { // TODO Dereference pointers
            } else {
                ast::base_node_ptr expression = parse_expression();
                statements.push_back(expression);
                expect(TOKEN_TYPE::Semicolon, "parser::parse_block(): Expected ';' after expression");
            }
        }
        m_scope--;
        return std::make_shared<ast::body_node>(statements);
    }

    ast::base_node_ptr parser::parse_variable_declaration() {
        const std::string_view type = handle_type();
        if (type.empty() || !is_type(type)) {
            throw parser_error("parser::parse_variable_declaration(): Expected variable type");
        }

        const std::string_view name = consume().value;
        if (name.empty()) {
            throw parser_error("parser::parse_variable_declaration(): Expected variable name");
        }

        if (m_scope.is_declared(name)) {
            throw parser_error("parser::parse_variable_declaration(): Variable already declared");
        }

        expect(TOKEN_TYPE::Semicolon, "parser::parse_variable_declaration(): Expected ';' after variable declaration");
        m_scope.add(name);
        return std::make_shared<ast::variable_declaration_node>(name, type);
    }

    ast::base_node_ptr parser::parse_variable_declaration_assign() {
        const std::string_view type = handle_type();
        if (type.empty() || !is_type(type)) {
            throw parser_error("parser::parse_variable_declaration_assign(): Expected variable type");
        }

        const std::string_view name = consume().value;
        if (name.empty()) {
            throw parser_error("parser::parse_variable_declaration_assign(): Expected variable name");
        }

        if (m_scope.is_declared(name)) {
            throw parser_error("parser::parse_variable_declaration_assign(): Variable already declared");
        }

        expect(TOKEN_TYPE::Assign, "parser::parse_variable_declaration_assign(): Expected '=' after variable name");

        ast::base_node_ptr rhs = parse_expression();
        expect(TOKEN_TYPE::Semicolon, "parser::parse_variable_declaration_assign(): Expected ';' after variable assignment");

        m_scope.add(name);
        return std::make_shared<ast::variable_declaration_assign_node>(name, type, rhs);
    }

    ast::base_node_ptr parser::parse_while() {
        expect(TOKEN_TYPE::While, "parser::parse_while(): Expected 'while' keyword");

        expect(TOKEN_TYPE::LeftParen, "parser::parse_while(): Expected '(' after 'while'");
        ast::base_node_ptr condition = parse_expression();
        expect(TOKEN_TYPE::RightParen, "parser::parse_while(): Expected ')' after condition");

        expect(TOKEN_TYPE::LeftBrace, "parser::parse_while(): Expected '{' after 'while' condition");
        ast::base_node_ptr body = parse_block();
        expect(TOKEN_TYPE::RightBrace, "parser::parse_while(): Expected '}' to close 'while' block");

        return std::make_shared<ast::while_node>(condition, body);
    }

    ast::base_node_ptr parser::parse_if() {
        expect(TOKEN_TYPE::If, "parser::parse_if(): Expected 'if' keyword");

        expect(TOKEN_TYPE::LeftParen, "parser::parse_if(): Expected '(' after 'if'");
        ast::base_node_ptr condition = parse_expression();
        expect(TOKEN_TYPE::RightParen, "parser::parse_if(): Expected ')' after condition");

        expect(TOKEN_TYPE::LeftBrace, "parser::parse_if(): Expected '{' after 'if' condition");
        ast::base_node_ptr true_body = parse_block();
        expect(TOKEN_TYPE::RightBrace, "parser::parse_if(): Expected '}' to close 'if' block");

        ast::base_node_ptr false_body = nullptr;
        if (check(TOKEN_TYPE::Else)) {
            consume();

            if (check(TOKEN_TYPE::If)) {
                false_body = parse_if();
            } else {
                expect(TOKEN_TYPE::LeftBrace, "parser::parse_if(): Expected '{' after 'else'");
                false_body = parse_block();
                expect(TOKEN_TYPE::RightBrace, "parser::parse_if(): Expected '}' to close 'else' block");
            }
        }

        return std::make_shared<ast::if_node>(condition, true_body, false_body);
    }

    ast::base_node_ptr parser::parse_switch() {
        expect(TOKEN_TYPE::Switch, "parser::parse_switch(): Expected 'switch' keyword");
        expect(TOKEN_TYPE::LeftParen, "parser::parse_switch(): Expected '(' after 'switch'");
        ast::base_node_ptr condition = parse_expression();
        expect(TOKEN_TYPE::RightParen, "parser::parse_switch(): Expected ')' after condition");
        expect(TOKEN_TYPE::LeftBrace, "parser::parse_switch(): Expected '{' after 'switch'");

        std::vector<ast::base_node_ptr> cases;
        ast::base_node_ptr default_case = nullptr;

        while (!check(TOKEN_TYPE::RightBrace)) {
            if (check(TOKEN_TYPE::Default)) {
                if (default_case) {
                    throw parser_error("parser::parse_switch(): Multiple 'default' cases in 'switch'");
                }
                default_case = parse_case();
            } else {
                cases.push_back(parse_case());
            }
        }

        expect(TOKEN_TYPE::RightBrace, "parser::parse_switch(): Expected '}' to close 'switch' block");
        return std::make_shared<ast::switch_node>(condition, cases, default_case);
    }

    ast::base_node_ptr parser::parse_function_call() {
        const std::string_view function_name = consume().value;
        if (function_name.empty()) {
            throw parser_error("parser::parse_function_call(): Expected function name");
        }

        expect(TOKEN_TYPE::LeftParen, "parser::parse_function_call(): Expected '(' after function name");
        std::vector<ast::base_node_ptr> arguments;

        if (!check(TOKEN_TYPE::RightParen)) {
            do {
                arguments.push_back(parse_expression());
            } while (check(TOKEN_TYPE::Comma) && consume().type == TOKEN_TYPE::Comma);
        }

        expect(TOKEN_TYPE::RightParen, "parser::parse_function_call(): Expected ')' after arguments");
        return std::make_shared<ast::function_call_node>(function_name, arguments);
    }

    ast::base_node_ptr parser::parse_case() {
        if (check(TOKEN_TYPE::Case)) {
            consume();
            ast::base_node_ptr value = parse_expression();
            expect(TOKEN_TYPE::Colon, "parser::parse_case(): Expected ':' after case value");
            std::vector<ast::base_node_ptr> body;
            while (!check(TOKEN_TYPE::Case) && !check(TOKEN_TYPE::Default) && !check(TOKEN_TYPE::RightBrace)) {
                body.push_back(parse_block());
            }
            return std::make_shared<ast::case_node>(value, std::make_shared<ast::body_node>(body));
        }

        if (check(TOKEN_TYPE::Default)) {
            consume();
            expect(TOKEN_TYPE::Colon, "parser::parse_case(): Expected ':' after 'default'");
            std::vector<ast::base_node_ptr> body;
            while (!check(TOKEN_TYPE::Case) && !check(TOKEN_TYPE::Default) && !check(TOKEN_TYPE::RightBrace)) {
                body.push_back(parse_block());
            }
            return std::make_shared<ast::case_node>(nullptr, std::make_shared<ast::body_node>(body));
        }

        throw parser_error("parser::parse_case(): Expected 'case' or 'default'");
    }

    ast::base_node_ptr parser::parse_expression() {
        std::function<ast::base_node_ptr()> parse_unary;
        std::function<ast::base_node_ptr()> parse_primary;

        parse_unary = [&]() -> ast::base_node_ptr {
            if (check(TOKEN_TYPE::Plus) || check(TOKEN_TYPE::Minus) || check(TOKEN_TYPE::Exclamation)) {
                auto op = consume().type;
                return std::make_shared<ast::unary_node>(op, parse_unary());
            }
            return parse_primary();
        };

        parse_primary = [&]() -> ast::base_node_ptr {
            if (check(TOKEN_TYPE::LeftParen)) {
                consume();
                auto expr = parse_expression();
                expect(TOKEN_TYPE::RightParen, "Expected ')'");
                return expr;
            }
            if (check(TOKEN_TYPE::Identifier)) {
                std::string_view identifier = consume().value;
                auto node = std::static_pointer_cast<ast::base_node>(std::make_shared<ast::variable_node>(identifier));

                while (check(TOKEN_TYPE::Period)) {
                    consume();
                    std::string_view member_name = consume().value;

                    if (check(TOKEN_TYPE::LeftParen)) {
                        consume();
                        std::vector<ast::base_node_ptr> arguments;
                        if (!check(TOKEN_TYPE::RightParen)) {
                            do {
                                arguments.push_back(parse_expression());
                            } while (check(TOKEN_TYPE::Comma) && consume().type == TOKEN_TYPE::Comma);
                        }
                        expect(TOKEN_TYPE::RightParen, "Expected ')'");
                        arguments.insert(arguments.begin(), node);
                        node = std::static_pointer_cast<ast::base_node>(
                            std::make_shared<ast::element_call_node>(member_name, arguments));
                    } else {
                        node = std::static_pointer_cast<ast::base_node>(
                            std::make_shared<ast::member_invoke_node>(node, member_name));
                    }
                }
                return node;
            }
            if (check(TOKEN_TYPE::Decimal) || check(TOKEN_TYPE::Hexadecimal) || check(TOKEN_TYPE::Binary)) {
                return parse_literal();
            }
            if (check(TOKEN_TYPE::StringLiteral)) {
                return std::make_shared<ast::string_literal_node>(consume().value);
            }
            throw parser_error("Unexpected token in primary expression");
        };

        auto parse_binary = [&](const int precedence, auto&& self) -> ast::base_node_ptr {
            auto left = parse_unary();
            while (true) {
                int current_precedence = get_precedence(peak().type);
                if (current_precedence < precedence) break;
                auto op = consume().type;
                auto right = self(current_precedence + 1, self);
                left = std::make_shared<ast::binary_node>(left, op, right);
            }
            return left;
        };

        return parse_binary(0, parse_binary);
    }


    ast::base_node_ptr parser::parse_literal() {
        if (check(TOKEN_TYPE::Decimal)) {
            return std::make_shared<ast::literal_node>(consume().value, ast::literal_node::LITERAL_TYPE::Decimal);
        }
        if (check(TOKEN_TYPE::Hexadecimal)) {
            return std::make_shared<ast::literal_node>(consume().value, ast::literal_node::LITERAL_TYPE::Hexadecimal);
        }
        if (check(TOKEN_TYPE::Binary)) {
            return std::make_shared<ast::literal_node>(consume().value, ast::literal_node::LITERAL_TYPE::Binary);
        }
        throw parser_error("Unexpected literal type");
    }

} // ent