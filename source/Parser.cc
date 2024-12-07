#include "Parser.hh"

namespace ent {
    const lexer::token& parser::peek(const size_t offset) const {
        if (m_current + offset < m_tokens.size()) {
            return m_tokens[m_current + offset];
        }
        return m_tokens.back();
    }

    const lexer::token& parser::current() const {
        return peek(0);
    }

    const lexer::token& parser::previous() const {
        return m_tokens[m_current - 1];
    }

    bool parser::match(const lexer::token::TOKEN_TYPE type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    bool parser::check(const lexer::token::TOKEN_TYPE type) const {
        if (is_at_end()) return false;
        return peek().type == type;
    }

    void parser::advance() {
        if (!is_at_end()) m_current++;
    }

    void parser::consume(const lexer::token::TOKEN_TYPE type, const std::string_view message) {
        if (check(type)) {
            advance();
            return;
        }
        error(current(), std::format("{} got: {}", message, peek().to_string()));
    }

    [[noreturn]] void parser::error(const lexer::token& tok, const std::string_view message) {
        throw parser_error(std::string(message), tok.line, tok.column);
    }

    bool parser::is_at_end() const {
        return current().type == lexer::token::TOKEN_TYPE::EOFToken;
    }

    ast::base_node_ptr parser::parse_program() {
        std::vector<ast::base_node_ptr> elements;

        while (!is_at_end()) {
            elements.push_back(parse_top_level_decl());
        }

        return std::make_shared<ast::program_node>(std::move(elements));
    }

    // Distinguish between:
    // extern fn name(...) -> type;         (extern foreign function)
    // fn name(...) -> type;                (forward-declared function)
    // fn name(...) -> type { ... }         (defined function)
    // extern type name;                    (extern global variable)
    // type name; / type name = expr;       (global variable)
    ast::base_node_ptr parser::parse_top_level_decl() {
        if (match(lexer::token::TOKEN_TYPE::Extern)) {
            if (match(lexer::token::TOKEN_TYPE::Function)) {
                // extern fn name(...) -> type;
                return parse_function_prototype(true);
            }
            // extern type name; a global extern variable
            return parse_global_variable(true);
        }

        if (match(lexer::token::TOKEN_TYPE::Function)) {
            // fn name(...) -> type; or fn name(...) -> type { ... }
            return parse_function(false);
        }

        // Otherwise, must be a global variable (type name[=expr];)
        if (is_type_keyword(current())) {
            return parse_global_variable(false);
        }

        error(current(), "Unexpected token at top level. Expected extern, fn, or a type for a global variable.");
    }

    // Parse a function (either forward-declared or defined) when we've already consumed 'fn'.
    // format: fn name(params) -> type; or fn name(params) -> type { ... }
    ast::base_node_ptr parser::parse_function(bool is_extern) {
        consume(lexer::token::TOKEN_TYPE::Identifier, "Expected function name after 'fn'.");
        const std::string_view name = previous().value;

        consume(lexer::token::TOKEN_TYPE::LeftParen, "Expected '(' after function name.");
        std::vector<ast::base_node_ptr> parameters;

        // parameters: (type name, type name, ...)
        if (!check(lexer::token::TOKEN_TYPE::RightParen)) {
            do {
                auto ptype = parse_type();
                consume(lexer::token::TOKEN_TYPE::Identifier, "Expected parameter name.");
                std::string_view pname = previous().value;
                parameters.push_back(std::make_shared<ast::parameter_node>(pname, ptype));
            } while (match(lexer::token::TOKEN_TYPE::Comma));
        }

        consume(lexer::token::TOKEN_TYPE::RightParen, "Expected ')' after parameters.");
        consume(lexer::token::TOKEN_TYPE::Minus, "Expected '->' after function parameters.");
        consume(lexer::token::TOKEN_TYPE::Greater, "Expected '->' after function parameters.");
        auto rtype = parse_type();

        // Now check if it's a definition or just a declaration
        if (match(lexer::token::TOKEN_TYPE::Semicolon)) {
            // forward-declared function with mangling
            return std::make_shared<ast::function_prototype_node>(rtype, name, parameters);
        }
        // must be a definition
        consume(lexer::token::TOKEN_TYPE::LeftBrace, "Expected '{' to start function body.");
        auto body = parse_block();
        consume(lexer::token::TOKEN_TYPE::Semicolon, "Expected ';' after function body");
        return std::make_shared<ast::function_node>(rtype, name, parameters, body);
    }

    // parse_function_prototype for extern function:
    // extern fn name(...) -> type; or fn name(..) -> type;
    ast::base_node_ptr parser::parse_function_prototype(bool is_extern) {
        consume(lexer::token::TOKEN_TYPE::Identifier, "Expected function name after 'fn'.");
        const std::string_view name = previous().value;

        consume(lexer::token::TOKEN_TYPE::LeftParen, "Expected '(' after function name.");
        std::vector<ast::base_node_ptr> parameters;
        if (!check(lexer::token::TOKEN_TYPE::RightParen)) {
            do {
                auto ptype = parse_type();
                consume(lexer::token::TOKEN_TYPE::Identifier, "Expected parameter name.");
                std::string_view pname = previous().value;
                parameters.push_back(std::make_shared<ast::parameter_node>(pname, ptype));
            } while (match(lexer::token::TOKEN_TYPE::Comma));
        }
        consume(lexer::token::TOKEN_TYPE::RightParen, "Expected ')' after parameters.");
        consume(lexer::token::TOKEN_TYPE::Minus, "Expected '->' after function parameters.");
        consume(lexer::token::TOKEN_TYPE::Greater, "Expected '->' after function parameters.");
        auto rtype = parse_type();
        consume(lexer::token::TOKEN_TYPE::Semicolon, "Expected ';' after extern function prototype.");
        return std::make_shared<ast::extern_node>(
                std::make_shared<ast::function_prototype_node>(rtype, name, parameters)
        );
    }

    // parse_global_variable:
    // If is_extern == true:
    //    extern type name;
    // else:
    //    type name; or type name = expr;
    ast::base_node_ptr parser::parse_global_variable(const bool is_extern) {
        auto vtype = parse_type();
        consume(lexer::token::TOKEN_TYPE::Identifier, "Expected variable name.");
        std::string_view name = previous().value;

        ast::base_node_ptr init = nullptr;
        if (is_extern) {
            // extern type name; no initialization allowed
            consume(lexer::token::TOKEN_TYPE::Semicolon, "Expected ';' after extern variable.");
            auto var_decl = std::make_shared<ast::variable_declaration_node>(name, vtype);
            // wrap in extern_node
            return std::make_shared<ast::extern_node>(var_decl);
        }
        // type name [= expr];

        if (match(lexer::token::TOKEN_TYPE::Assign)) {
            init = parse_expression();
        }
        consume(lexer::token::TOKEN_TYPE::Semicolon, "Expected ';' after global variable declaration.");
        if (init) {
            return std::make_shared<ast::variable_declaration_assign_node>(name, vtype, init);
        }
        return std::make_shared<ast::variable_declaration_node>(name, vtype);
    }

    variable_type parser::parse_type() {
        if (!is_type_keyword(current())) {
            error(current(), "Expected type keyword.");
        }
        const std::string_view base = current().value;
        advance();
        int ptr_count = 0;
        while (match(lexer::token::TOKEN_TYPE::Star)) {
            ptr_count++;
        }
        variable_type vtype;
        vtype.base_type = base;
        vtype.pointer = ptr_count;
        vtype.is_struct = false; // for now (struct are going to be ouchy)
        return vtype;
    }

    bool parser::is_type_keyword(const lexer::token& tok) {
        switch (tok.type) {
            case lexer::token::TOKEN_TYPE::Void:
            case lexer::token::TOKEN_TYPE::Byte:
            case lexer::token::TOKEN_TYPE::Word:
            case lexer::token::TOKEN_TYPE::DWord:
            case lexer::token::TOKEN_TYPE::QWord:
            case lexer::token::TOKEN_TYPE::SByte:
            case lexer::token::TOKEN_TYPE::SWord:
            case lexer::token::TOKEN_TYPE::SDWord:
            case lexer::token::TOKEN_TYPE::SQWord:
                return true;
            default:
                return false;
        }
    }

    // === Statements & Blocks ===
    ast::base_node_ptr parser::parse_statement() {
        if (match(lexer::token::TOKEN_TYPE::If)) return parse_if_statement();
        if (match(lexer::token::TOKEN_TYPE::While)) return parse_while_statement();
        if (match(lexer::token::TOKEN_TYPE::Switch)) return parse_switch_statement();
        if (match(lexer::token::TOKEN_TYPE::Return)) return parse_return_statement();
        if (match(lexer::token::TOKEN_TYPE::Break)) return parse_break_statement();
        if (match(lexer::token::TOKEN_TYPE::Continue)) return parse_continue_statement();

        if (is_type_keyword(current())) {
            return parse_variable_declaration(false);
        }

        auto expr = parse_expression();
        consume(lexer::token::TOKEN_TYPE::Semicolon, "Expected ';' after expression.");
        return expr;
    }

    ast::base_node_ptr parser::parse_block() {
        std::vector<ast::base_node_ptr> statements;
        while (!check(lexer::token::TOKEN_TYPE::RightBrace) && !is_at_end()) {
            statements.push_back(parse_statement());
        }
        consume(lexer::token::TOKEN_TYPE::RightBrace, "Expected '}' after block.");
        return std::make_shared<ast::body_node>(std::move(statements));
    }

    ast::base_node_ptr parser::parse_if_statement() {
        consume(lexer::token::TOKEN_TYPE::LeftParen, "Expected '(' after 'if'.");
        auto condition = parse_expression();
        consume(lexer::token::TOKEN_TYPE::RightParen, "Expected ')' after if condition.");
        consume(lexer::token::TOKEN_TYPE::LeftBrace, "Expected '{' after if condition.");
        auto true_body = parse_block();

        ast::base_node_ptr false_body = nullptr;

        while (check(lexer::token::TOKEN_TYPE::Else) && (peek(1).type == lexer::token::TOKEN_TYPE::If)) {
            consume(lexer::token::TOKEN_TYPE::Else, "Expected 'else' before 'if' in 'else if'.");
            consume(lexer::token::TOKEN_TYPE::If, "Expected 'if' after 'else'.");

            consume(lexer::token::TOKEN_TYPE::LeftParen, "Expected '(' after 'else if'.");
            auto else_if_condition = parse_expression();
            consume(lexer::token::TOKEN_TYPE::RightParen, "Expected ')' after else if condition.");
            consume(lexer::token::TOKEN_TYPE::LeftBrace, "Expected '{' after else if condition.");
            auto else_if_body = parse_block();

            auto else_if_node = std::make_shared<ast::if_node>(else_if_condition, else_if_body, nullptr);

            if (!false_body) {
                false_body = else_if_node;
            } else {
                auto current = std::dynamic_pointer_cast<ast::if_node>(false_body);
                while (current->false_body() && current->false_body()->type() == ast::NODE_TYPE::If) {
                    current = std::dynamic_pointer_cast<ast::if_node>(current->false_body());
                }
                current->set_false_body(else_if_node);
            }
        }

        if (match(lexer::token::TOKEN_TYPE::Else)) {
            consume(lexer::token::TOKEN_TYPE::LeftBrace, "Expected '{' after 'else'.");
            const auto else_body = parse_block();

            if (!false_body) {
                false_body = else_body;
            } else {
                auto current = std::dynamic_pointer_cast<ast::if_node>(false_body);
                while (current->false_body() && current->false_body()->type() == ast::NODE_TYPE::If) {
                    current = std::dynamic_pointer_cast<ast::if_node>(current->false_body());
                }
                current->set_false_body(else_body);
            }
        }

        return std::make_shared<ast::if_node>(condition, true_body, false_body);
    }


    ast::base_node_ptr parser::parse_while_statement() {
        consume(lexer::token::TOKEN_TYPE::LeftParen, "Expected '(' after 'while'.");
        auto condition = parse_expression();
        consume(lexer::token::TOKEN_TYPE::RightParen, "Expected ')' after while condition.");
        consume(lexer::token::TOKEN_TYPE::LeftBrace, "Expected '{' after while condition.");
        auto body = parse_block();
        return std::make_shared<ast::while_node>(condition, body);
    }

    ast::base_node_ptr parser::parse_switch_statement() {
        consume(lexer::token::TOKEN_TYPE::LeftParen, "Expected '(' after 'switch'.");
        auto expr = parse_expression();
        consume(lexer::token::TOKEN_TYPE::RightParen, "Expected ')' after switch expression.");
        consume(lexer::token::TOKEN_TYPE::LeftBrace, "Expected '{' after switch.");

        std::vector<ast::base_node_ptr> cases;
        ast::base_node_ptr default_case = nullptr;
        while (!check(lexer::token::TOKEN_TYPE::RightBrace) && !is_at_end()) {
            if (match(lexer::token::TOKEN_TYPE::Case)) {
                cases.push_back(parse_case_statement());
            } else if (match(lexer::token::TOKEN_TYPE::Default)) {
                consume(lexer::token::TOKEN_TYPE::Colon, "Expected ':' after 'default'.");
                std::vector<ast::base_node_ptr> stmts;
                while (!check(lexer::token::TOKEN_TYPE::Case) &&
                       !check(lexer::token::TOKEN_TYPE::Default) &&
                       !check(lexer::token::TOKEN_TYPE::RightBrace) &&
                       !is_at_end()) {
                    stmts.push_back(parse_statement());
                }
                default_case = std::make_shared<ast::body_node>(std::move(stmts));
            } else {
                error(current(), "Expected 'case' or 'default' in switch.");
            }
        }

        consume(lexer::token::TOKEN_TYPE::RightBrace, "Expected '}' after switch.");
        return std::make_shared<ast::switch_node>(expr, cases, default_case);
    }

    ast::base_node_ptr parser::parse_case_statement() {
        auto val_expr = parse_expression();
        consume(lexer::token::TOKEN_TYPE::Colon, "Expected ':' after case value.");
        std::vector<ast::base_node_ptr> stmts;
        while (!check(lexer::token::TOKEN_TYPE::Case) &&
               !check(lexer::token::TOKEN_TYPE::Default) &&
               !check(lexer::token::TOKEN_TYPE::RightBrace) &&
               !is_at_end()) {
            stmts.push_back(parse_statement());
        }
        return std::make_shared<ast::case_node>(val_expr, std::make_shared<ast::body_node>(std::move(stmts)));
    }

    ast::base_node_ptr parser::parse_return_statement() {
        if (!check(lexer::token::TOKEN_TYPE::Semicolon)) {
            auto val = parse_expression();
            consume(lexer::token::TOKEN_TYPE::Semicolon, "Expected ';' after return value.");
            return std::make_shared<ast::return_node>(val);
        }
        advance();
        return std::make_shared<ast::return_node>(nullptr);
    }

    ast::base_node_ptr parser::parse_break_statement() {
        consume(lexer::token::TOKEN_TYPE::Semicolon, "Expected ';' after 'break'.");
        return std::make_shared<ast::break_node>();
    }

    ast::base_node_ptr parser::parse_continue_statement() {
        consume(lexer::token::TOKEN_TYPE::Semicolon, "Expected ';' after 'continue'.");
        return std::make_shared<ast::continue_node>();
    }

    // type name [= expr];
    ast::base_node_ptr parser::parse_variable_declaration(bool allow_extern) {
        const auto vtype = parse_type();
        consume(lexer::token::TOKEN_TYPE::Identifier, "Expected variable name after type.");
        std::string_view name = previous().value;

        ast::base_node_ptr init = nullptr;
        if (match(lexer::token::TOKEN_TYPE::Assign)) {
            init = parse_expression();
        }
        consume(lexer::token::TOKEN_TYPE::Semicolon, "Expected ';' after variable declaration.");
        if (init) {
            return std::make_shared<ast::variable_declaration_assign_node>(name, vtype, init);
        }
        return std::make_shared<ast::variable_declaration_node>(name, vtype);
    }

    // === Expressions ===
    ast::base_node_ptr parser::parse_expression() {
        return parse_assignment_expr();
    }

    ast::base_node_ptr parser::parse_assignment_expr() {
        auto lhs = parse_logical_or_expr();

        if (match(lexer::token::TOKEN_TYPE::Assign)) {
            auto rhs = parse_assignment_expr();

            const auto var = std::dynamic_pointer_cast<ast::variable_node>(lhs);
            if (!var) {
                error(current(), "Left-hand side of assignment must be assignable.");
            }

            return std::make_shared<ast::assignment_node>(var->get_name(), rhs);
        }

        return lhs;
    }

    ast::base_node_ptr parser::parse_logical_or_expr() {
        auto node = parse_logical_and_expr();
        while (match(lexer::token::TOKEN_TYPE::Pipe) && match(lexer::token::TOKEN_TYPE::Pipe)) {
            auto right = parse_logical_and_expr();
            node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::LOGICAL_OR, right);
        }
        return node;
    }

    ast::base_node_ptr parser::parse_logical_and_expr() {
        auto node = parse_equality_expr();
        while (match(lexer::token::TOKEN_TYPE::Ampersand) && match(lexer::token::TOKEN_TYPE::Ampersand)) {
            auto right = parse_equality_expr();
            node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::LOGICAL_AND, right);
        }
        return node;
    }

    ast::base_node_ptr parser::parse_equality_expr() {
        auto node = parse_relational_expr();
        while (true) {
            if (match(lexer::token::TOKEN_TYPE::Equal)) {
                auto right = parse_relational_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::EQUAL, right);
            } else if (match(lexer::token::TOKEN_TYPE::NotEqual)) {
                auto right = parse_relational_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::NOT_EQUAL, right);
            } else {
                break;
            }
        }
        return node;
    }

    ast::base_node_ptr parser::parse_relational_expr() {
        auto node = parse_additive_expr();
        while (true) {
            if (match(lexer::token::TOKEN_TYPE::Less)) {
                auto right = parse_additive_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::LESS, right);
            } else if (match(lexer::token::TOKEN_TYPE::LessEqual)) {
                auto right = parse_additive_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::LESS_EQUAL, right);
            } else if (match(lexer::token::TOKEN_TYPE::Greater)) {
                auto right = parse_additive_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::GREATER, right);
            } else if (match(lexer::token::TOKEN_TYPE::GreaterEqual)) {
                auto right = parse_additive_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::GREATER_EQUAL, right);
            } else {
                break;
            }
        }
        return node;
    }

    ast::base_node_ptr parser::parse_additive_expr() {
        auto node = parse_multiplicative_expr();
        while (true) {
            if (match(lexer::token::TOKEN_TYPE::Plus)) {
                auto right = parse_multiplicative_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::ADDITION, right);
            } else if (match(lexer::token::TOKEN_TYPE::Minus)) {
                auto right = parse_multiplicative_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::SUBTRACTION, right);
            } else {
                break;
            }
        }
        return node;
    }

    ast::base_node_ptr parser::parse_multiplicative_expr() {
        auto node = parse_unary_expr();
        while (true) {
            if (match(lexer::token::TOKEN_TYPE::Star)) {
                auto right = parse_unary_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::MULTIPLICATION, right);
            } else if (match(lexer::token::TOKEN_TYPE::Slash)) {
                auto right = parse_unary_expr();
                node = std::make_shared<ast::expression_node>(node, ast::EXPRESSION_NODE_OP::DIVISION, right);
            } else {
                break;
            }
        }
        return node;
    }

    ast::base_node_ptr parser::parse_unary_expr() {
        if (match(lexer::token::TOKEN_TYPE::Increment)) {
            const auto operand = parse_unary_expr();
            const auto var = std::dynamic_pointer_cast<ast::variable_node>(operand);
            if (!var) {
                error(current(), "Prefix ++ operator applied to a non-variable expression.");
            }
            return std::make_shared<ast::increment_node>(var->get_name(), true);
        }
        if (match(lexer::token::TOKEN_TYPE::Decrement)) {
            const auto operand = parse_unary_expr();
            const auto var = std::dynamic_pointer_cast<ast::variable_node>(operand);
            if (!var) {
                error(current(), "Prefix -- operator applied to a non-variable expression.");
            }
            return std::make_shared<ast::decrement_node>(var->get_name(), true);
        }

        if (match(lexer::token::TOKEN_TYPE::Minus) ||
            match(lexer::token::TOKEN_TYPE::Plus) ||
            match(lexer::token::TOKEN_TYPE::Exclamation) ||
            match(lexer::token::TOKEN_TYPE::Ampersand) ||
            match(lexer::token::TOKEN_TYPE::Star)) {
            auto op = previous().type;
            auto operand = parse_unary_expr();
            return std::make_shared<ast::unary_node>(op, operand);
        }
        return parse_primary_expr();
    }

    ast::base_node_ptr parser::parse_primary_expr() {
        if (match(lexer::token::TOKEN_TYPE::LeftParen)) {
            auto expr = parse_expression();
            consume(lexer::token::TOKEN_TYPE::RightParen, "Expected ')' after expression.");
            expr = parse_postfix_operators(expr);
            return expr;
        }

        if (match(lexer::token::TOKEN_TYPE::Decimal)) {
            auto node = dynamic_pointer_cast<ast::base_node>(std::make_shared<ast::literal_node>(previous().value, ast::literal_node::LITERAL_TYPE::Decimal));
            node = parse_postfix_operators(node);
            return node;
        }

        if (match(lexer::token::TOKEN_TYPE::Hexadecimal)) {
            auto node = dynamic_pointer_cast<ast::base_node>(std::make_shared<ast::literal_node>(previous().value, ast::literal_node::LITERAL_TYPE::Hexadecimal));
            node = parse_postfix_operators(node);
            return node;
        }

        if (match(lexer::token::TOKEN_TYPE::Binary)) {
            auto node = dynamic_pointer_cast<ast::base_node>(std::make_shared<ast::literal_node>(previous().value, ast::literal_node::LITERAL_TYPE::Binary));
            node = parse_postfix_operators(node);
            return node;
        }

        if (match(lexer::token::TOKEN_TYPE::StringLiteral)) {
            auto node = dynamic_pointer_cast<ast::base_node>(std::make_shared<ast::string_literal_node>(previous().value));
            node = parse_postfix_operators(node);
            return node;
        }

        if (match(lexer::token::TOKEN_TYPE::Identifier)) {
            auto node = parse_function_call_or_variable();
            node = parse_postfix_operators(node);
            return node;
        }

        error(current(), std::format("Unexpected token in expression; got {}", previous().to_string()));
        return nullptr;
    }

    ast::base_node_ptr parser::parse_function_call_or_variable() {
        auto name = previous().value;

        ast::base_node_ptr node;
        if (match(lexer::token::TOKEN_TYPE::LeftParen)) {
            std::vector<ast::base_node_ptr> args;
            if (!check(lexer::token::TOKEN_TYPE::RightParen)) {
                do {
                    args.push_back(parse_expression());
                } while (match(lexer::token::TOKEN_TYPE::Comma));
            }
            consume(lexer::token::TOKEN_TYPE::RightParen, "Expected ')' after arguments.");
            node = std::make_shared<ast::function_call_node>(name, std::move(args));
        } else {
            node = std::make_shared<ast::variable_node>(name);
        }

        while (true) {
            if (match(lexer::token::TOKEN_TYPE::LeftBracket)) {
                // Indexing: var[idx]
                auto idx = parse_expression();
                consume(lexer::token::TOKEN_TYPE::RightBracket, "Expected ']' after index.");
                node = std::make_shared<ast::index_access_node>(std::dynamic_pointer_cast<ast::variable_node>(node)->get_name(), idx);
            } else if (match(lexer::token::TOKEN_TYPE::Period)) {
                // Member access: node.member or node.member(...)
                consume(lexer::token::TOKEN_TYPE::Identifier, "Expected member name after '.'.");
                std::string_view member = previous().value;

                // UFCS? (and todo struct's function pointers)
                if (check(lexer::token::TOKEN_TYPE::LeftParen)) {
                    advance();
                    std::vector<ast::base_node_ptr> args;

                    args.push_back(node);

                    if (!check(lexer::token::TOKEN_TYPE::RightParen)) {
                        do {
                            args.push_back(parse_expression());
                        } while (match(lexer::token::TOKEN_TYPE::Comma));
                    }
                    consume(lexer::token::TOKEN_TYPE::RightParen, "Expected ')' after arguments.");

                    node = std::make_shared<ast::function_call_node>(member, std::move(args));
                } else {
                    node = std::make_shared<ast::member_invoke_node>(node, member);
                }
            } else {
                break;
            }
        }

        return node;
    }


    ast::EXPRESSION_NODE_OP parser::token_to_expression_op(const lexer::token::TOKEN_TYPE type) {
        switch (type) {
            case lexer::token::TOKEN_TYPE::Plus: return ast::EXPRESSION_NODE_OP::ADDITION;
            case lexer::token::TOKEN_TYPE::Minus: return ast::EXPRESSION_NODE_OP::SUBTRACTION;
            case lexer::token::TOKEN_TYPE::Star: return ast::EXPRESSION_NODE_OP::MULTIPLICATION;
            case lexer::token::TOKEN_TYPE::Slash: return ast::EXPRESSION_NODE_OP::DIVISION;
            default: return ast::EXPRESSION_NODE_OP::ADDITION; // fallback
        }
    }

    bool parser::is_unary_operator(const lexer::token& tok) {
        return tok.type == lexer::token::TOKEN_TYPE::Plus ||
               tok.type == lexer::token::TOKEN_TYPE::Minus ||
                tok.type == lexer::token::TOKEN_TYPE::Exclamation ||
               tok.type == lexer::token::TOKEN_TYPE::Ampersand ||
               tok.type == lexer::token::TOKEN_TYPE::Star;
    }

    bool parser::is_binary_operator(const lexer::token& tok) {
        switch (tok.type) {
            case lexer::token::TOKEN_TYPE::Plus:
            case lexer::token::TOKEN_TYPE::Minus:
            case lexer::token::TOKEN_TYPE::Star:
            case lexer::token::TOKEN_TYPE::Slash:
            case lexer::token::TOKEN_TYPE::Equal:
            case lexer::token::TOKEN_TYPE::NotEqual:
            case lexer::token::TOKEN_TYPE::Less:
            case lexer::token::TOKEN_TYPE::LessEqual:
            case lexer::token::TOKEN_TYPE::Greater:
            case lexer::token::TOKEN_TYPE::GreaterEqual:
                return true;
            default:
                return false;
        }
    }

    ast::base_node_ptr parser::parse_postfix_operators(ast::base_node_ptr expr) {
        // Postfix increments/decrements only make sense on variables or something that can be incremented.
        // We'll assume only variables can be incremented. If `expr` is not a variable, throw an error.
        // TODO relax this rule to support something like arr[i]++ etc. but then we would have to store
        // more than just a name in increment_node/decrement_node.

        while (true) {
            if (match(lexer::token::TOKEN_TYPE::Increment)) {
                // Postfix ++
                const auto var = std::dynamic_pointer_cast<ast::variable_node>(expr);
                if (!var) {
                    error(current(), "Postfix ++ operator applied to non-variable expression.");
                }
                expr = std::make_shared<ast::increment_node>(var->get_name(), false);
            } else if (match(lexer::token::TOKEN_TYPE::Decrement)) {
                // Postfix --
                const auto var = std::dynamic_pointer_cast<ast::variable_node>(expr);
                if (!var) {
                    error(current(), "Postfix -- operator applied to non-variable expression.");
                }
                expr = std::make_shared<ast::decrement_node>(var->get_name(), false);
            } else {
                break;
            }
        }
        return expr;
    }

} // namespace ent
