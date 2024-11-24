//
// Created by notbonzo on 11/19/24.
//
#include "Preprocessor.hh"
#include <string_view>
#include <fstream>
#include <regex>

namespace ent {
    preprocessor::preprocessor(const std::string_view filename) : m_filename(filename), m_file(filename.data()) {
        if (!m_file.is_open()) {
            throw file_not_found_error(filename);
        }
        static const std::regex header_start_regex(R"(^\s*header\s*\{)");
        static const std::regex define_regex(R"(^\s*define\s+\w+.*$)");
        static const std::regex include_regex(R"(^\s*include\s*["<](.*)[">]\s*)");
        // TODO: Add regex's for macro conditions

        while(std::getline(m_file, m_line)) {
            if (std::regex_search(m_line, header_start_regex)) {
                m_in_header_block = true;
                m_brace_balance = 1;
                m_header_content.clear();
                continue;
            }
            if (m_in_header_block) {
                if (std::regex_search(m_line, include_regex)) {
                    std::smatch match;
                    if (!std::regex_search(m_line, match, include_regex)) {
                        throw preprocessor_error(std::format("Failed to parse detected include statement in header at:\n{}\n", m_line));
                    }
                    std::string include_path = match[1];
                    if (m_includes.emplace(include_path).second) {
                        preprocessor prep(include_path);
                        const std::string included_content = prep.get_header();
                        m_header_content += included_content;
                        m_preprocessed_file += included_content;
                    } else {
                        throw preprocessor_error(std::format("Cyclic include detected for path: {}\n", include_path));
                    }
                } else {
                    m_brace_balance += count_braces(m_line);
                    if (m_brace_balance <= 0) {
                        std::string::size_type brace_pos = m_line.find('}');
                        while (brace_pos != std::string::npos && m_brace_balance <= 0) {
                            m_brace_balance++;
                            brace_pos = m_line.find('}', brace_pos + 1);
                        }
                        if (brace_pos != std::string::npos) {
                            m_preprocessed_file += m_line.substr(0, brace_pos) + '\n';
                            m_header_content += m_line.substr(0, brace_pos) + '\n';
                        }
                        m_in_header_block = false;
                        continue;
                    }
                    m_brace_balance += count_braces(m_line);
                    if (m_brace_balance <= 0) {
                        std::string::size_type brace_pos = m_line.find('}');
                        while (brace_pos != std::string::npos && m_brace_balance <= 0) {
                            m_brace_balance++;
                            brace_pos = m_line.find('}', brace_pos + 1);
                        }
                        if (brace_pos != std::string::npos) {
                            m_preprocessed_file += m_line.substr(0, brace_pos) + '\n';
                            m_header_content += m_line.substr(0, brace_pos) + '\n';
                        }
                        m_in_header_block = false;
                    } else {
                        m_preprocessed_file += m_line += '\n';
                        m_header_content += m_line += '\n';
                    }
                }
                continue;
            }
            if (std::regex_search(m_line, define_regex)) {
// TODO add macros at Iteration 2 of coding
            } else if (std::regex_search(m_line, include_regex)) {
                std::smatch match;
                if (!std::regex_search(m_line, match, include_regex)) {
                    throw preprocessor_error(std::format("Failed to parse detected include statement in header at:\n{}\n", m_line));
                }
                std::string include_path = match[1];
                if (m_includes.emplace(include_path).second) {
                    preprocessor prep(include_path);
                    m_preprocessed_file += prep.get_header();
                } else {
                    throw preprocessor_error(std::format("Include path is invalid in:\n{}\n", m_line));
                }
                continue;
            }
            m_preprocessed_file += m_line += '\n';
        }
        if (m_in_header_block && m_brace_balance != 0) {
            throw preprocessor_error("Unclosed header block detected in file: " + std::string(m_filename));
        }
    }

    std::string& preprocessor::get_preprocessed() noexcept {
        return m_preprocessed_file;
    }
    std::string& preprocessor::get_header() noexcept {
        return m_header_content;
    }

    int preprocessor::count_braces(const std::string_view line) {
        int balance = 0;
        for (const char c : line) {
            if (c == '{') {
                balance++;
            } else if (c == '}') {
                balance--;
            }
        }
        return balance;
    }
}