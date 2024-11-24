//
// Created by notbonzo on 11/19/24.
//

#ifndef PREPROCESSOR_HH
#define PREPROCESSOR_HH

#include "Error.hh"
#include <string_view>
#include <fstream>
#include <set>
#include <string>

namespace ent {
    class preprocessor_error : public error {
    public:
        explicit preprocessor_error(const std::string_view msg) : error(msg) {}
    };
    class file_not_found_error final : public preprocessor_error {
    public:
        explicit file_not_found_error(const std::string_view msg) : preprocessor_error(std::format("File not found: {}", msg)) {}
    };
    class preprocessor {
    public:
        explicit preprocessor(std::string_view filename);

        std::string& get_preprocessed() noexcept;
        std::string& get_header() noexcept;

    private:
        static int count_braces(std::string_view line);
        std::string_view m_filename;
        std::ifstream m_file;
        std::string m_preprocessed_file;

        std::string m_line;
        std::string m_header_content;
        std::set<std::string_view> m_includes;
        bool m_in_header_block = false;
        int m_brace_balance = 0;
    };
}

#endif //PREPROCESSOR_HH
