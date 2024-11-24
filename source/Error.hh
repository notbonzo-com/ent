//
// Created by notbonzo on 11/19/24.
//

#ifndef ERROR_HH
#define ERROR_HH

#include <stdexcept>
#include <string_view>
#include <format>
#include <string>

namespace ent {
    constexpr std::string_view ANSI_RESET = "\033[0m";
    constexpr std::string_view ANSI_BOLD_RED = "\033[1;31m";
    constexpr std::string_view ANSI_BOLD_YELLOW = "\033[1;33m";
    constexpr std::string_view ANSI_BOLD_WHITE = "\033[1;37m";

    class generic_error : public std::runtime_error {
    public:
        explicit generic_error(const std::string_view msg) : std::runtime_error(msg.data()), m_msg(msg) { }
        [[nodiscard]] const char* what() const noexcept override {
            return m_msg.data();
        }
    protected:
        std::string m_msg;
    };
    class fatal_error : public generic_error {
    public:
        explicit fatal_error(const std::string_view msg) : generic_error(std::format("{}ents: {}fatal error:{} {}\n{}compilation terminated.\n", ANSI_BOLD_WHITE, ANSI_BOLD_RED, ANSI_RESET, msg, ANSI_RESET)) {}
    };
    class error : public generic_error {
    public:
        explicit error(const std::string_view msg) : generic_error(std::format("{}ents: {}error:{} {}\n{}compilation terminated.\n", ANSI_BOLD_WHITE, ANSI_BOLD_RED, ANSI_RESET, msg, ANSI_RESET)) {}
    };
    class warning : public generic_error {
    public:
        explicit warning(const std::string_view msg) : generic_error(std::format("{}ents: {}warning:{} {}\n{}compilation terminated.\n", ANSI_BOLD_WHITE, ANSI_BOLD_YELLOW, ANSI_RESET, msg, ANSI_RESET)) {}
    };
}

#endif //ERROR_HH
