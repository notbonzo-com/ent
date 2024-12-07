//
// Created by notbonzo on 11/19/24.
//
#include "Preprocessor.hh"
#include <string_view>
#include <fstream>
#include <regex>
#include <format>

namespace ent {
    preprocessor::preprocessor(const std::string_view filename) : m_filename(filename), m_file(filename.data()) {
        if (!m_file.is_open()) {
            throw file_not_found_error(filename);
        }

        static const std::regex header_start_regex(R"(^\s*header\s*\{)");
        static const std::regex define_regex(R"(^\s*define\s+\w+.*$)");
        static const std::regex include_regex(R"(^\s*include\s*["<](.*)[">]\s*)");

        while (std::getline(m_file, m_line)) {
            // If we detect the start of a header block on this line
            if (std::regex_search(m_line, header_start_regex)) {
                // We have something like:
                //   header {
                // or header { something }
                // or header { ... multiple braces ... }
                //
                // First, find where the header block starts:
                const std::size_t start_pos = m_line.find('{');
                if (start_pos == std::string::npos) {
                    // Should not happen since regex matched, but just in case
                    throw preprocessor_error(std::format("Malformed header line detected: {}\n", m_line));
                }

                m_in_header_block = true;
                m_header_content.clear();
                m_brace_balance = 0;

                const int line_braces = count_braces(m_line);

                m_brace_balance += line_braces;

                // Extract the portion after "header {", because that's the start of the block content
                // We'll append the entire line (minus the 'header ' part) to header content and handle braces
                // But we must carefully handle where we start adding from:
                // We'll capture everything from the first '{' since that's where block content truly starts.
                std::string header_line_content = m_line.substr(start_pos + 1); // content after '{'

                // If after counting line_braces we have m_brace_balance == 0, that means
                // the block opened and closed on the same line.
                if (m_brace_balance == 0) {
                    // This means something like: header { something }
                    // We should isolate the content inside braces (which we already have as header_line_content).
                    // The closing brace should be somewhere in header_line_content. Let's remove it:
                    if (const std::size_t closing_brace = header_line_content.find('}'); closing_brace != std::string::npos) {
                        std::string content = header_line_content.substr(0, closing_brace);
                        // TODO Trim whitespace ?
                        m_header_content += content + '\n';
                        m_preprocessed_file += content + '\n';
                    }
                    // Block ended on the same line
                    m_in_header_block = false;
                } else {
                    // The block continues. The line may have started the block but not ended it.
                    // We'll include whatever content is after the '{' in the header block.
                    // There's a possibility that the line had multiple braces,
                    // so let's carefully handle it. Since we know we didn't close the block yet,
                    // we keep the line as part of the header and preprocessed file.
                    m_preprocessed_file += header_line_content + '\n';
                    m_header_content += header_line_content + '\n';
                }
                continue; // Next line
            }

            if (m_in_header_block) {
                // Check for includes inside header block
                // If found, process them recursively
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
                    const int line_braces = count_braces(m_line);
                    m_brace_balance += line_braces;

                    if (m_brace_balance <= 0) {
                        // This means we've closed the block on this line.
                        // We need to handle possibly multiple '}' in the line.
                        // Let's carefully find the brace that closed the block.

                        // We'll find the closing '}' that balances the block.
                        // Since line_braces can be negative if there are more closing braces than opens,
                        // we incrementally adjust until balance is back to zero.
                        int local_balance = line_braces;
                        std::size_t search_pos = 0;
                        std::string processed_line;
                        while (local_balance < 0 && search_pos < m_line.size()) {
                            const std::size_t brace_pos = m_line.find('}', search_pos);
                            if (brace_pos == std::string::npos)
                                break; // no more '}' found
                            // Each '}' increments by -1
                            local_balance++;
                            // We'll take the substring up to the brace that ended the block
                            if (local_balance == 0) {
                                // This is the brace that closed the header block
                                processed_line = m_line.substr(0, brace_pos);
                                break;
                            }
                            search_pos = brace_pos + 1;
                        }

                        // Add whatever content we got before closing
                        if (!processed_line.empty()) {
                            m_preprocessed_file += processed_line + '\n';
                            m_header_content += processed_line + '\n';
                        }

                        // Now the block is closed
                        m_in_header_block = false;
                    } else {
                        // Block still open
                        m_preprocessed_file += m_line + '\n';
                        m_header_content += m_line + '\n';
                    }
                }
                continue;
            }

            if (std::regex_search(m_line, define_regex)) {
                // TODO add macros at Iteration 2 of coding
            } else if (std::regex_search(m_line, include_regex)) {
                std::smatch match;
                if (!std::regex_search(m_line, match, include_regex)) {
                    throw preprocessor_error(std::format("Failed to parse detected include statement at:\n{}\n", m_line));
                }
                std::string include_path = match[1];
                if (m_includes.emplace(include_path).second) {
                    preprocessor prep(include_path);
                    m_preprocessed_file += prep.get_header();
                } else {
                    throw preprocessor_error(std::format("Include path is invalid or cyclic in:\n{}\n", m_line));
                }
                continue;
            }
            m_preprocessed_file += m_line + '\n';
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