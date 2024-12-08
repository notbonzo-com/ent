#include <print>
#include <fstream>
#include "Lexer.hh"
#include "Parser.hh"
#include "AST.icc"
#include "Preprocessor.hh"

void test_parser_with_file(const std::string& file_path) {
    std::print("Parsing file: {}\n", file_path);

    ent::preprocessor pp(file_path);
    const auto source = pp.get_preprocessed();
    ent::lexer lexer(source);
    const auto tokens = lexer.get_tokens();
    ent::parser parser(tokens);

    const auto ast = parser.parse_program();
    if (ast) {
        std::print("AST for {}:\n", file_path);
        ast->print(0);
    } else {
        std::print("No AST generated for {}\n", file_path);
    }

    std::print("Parsing successful for file: {}\n", file_path);

}

int main(const int argc, char* argv[]) {
    if (argc < 2) {
        std::print("Usage: {} <test files...>\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        test_parser_with_file(argv[i]);
    }

    std::print("All tests completed.\n");
    return 0;
}
