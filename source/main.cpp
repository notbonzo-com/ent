#include <print>
#include <fstream>
#include "Lexer.hh"
#include "Parser.hh"
#include "AST.icc"
#include "Preprocessor.hh"

void test_parser_with_file(const std::string& file_path) {
        std::print("Parsing file: {}\n", file_path);

        ent::preprocessor pp(file_path);
        auto source = pp.get_preprocessed();
        std::println(
            "Preprocessed:\n{}\n", source
        );
        ent::lexer lexer(source);
        auto tokens = lexer.get_tokens();
        ent::parser parser(tokens);

        auto ast = parser.parse();
        if (ast) {
            std::print("AST for {}:\n", file_path);
            ast->print(0);
        } else {
            std::print("No AST generated for {}\n", file_path);
        }

        std::print("Parsing successful for file: {}\n", file_path);
}

int main(int argc, char* argv[]) {
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
