#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

#if __has_feature(cxx_constexpr) 
    constexpr char ANSI_RESET[] = "\033[0m";
    constexpr char ANSI_BOLD_RED[] = "\033[1;31m";
    constexpr char ANSI_BOLD_YELLOW[] = "\033[1;33m";
    constexpr char ANSI_BOLD_WHITE[] = "\033[1;37m";
#else 
    #define ANSI_RESET       "\033[0m"
    #define ANSI_BOLD_RED    "\033[1;31m"
    #define ANSI_BOLD_YELLOW "\033[1;33m"
    #define ANSI_BOLD_WHITE  "\033[1;37m"
#endif

typedef enum {
    ERROR_LEVEL_FATAL,
    ERROR_LEVEL_ERROR,
    ERROR_LEVEL_WARNING
} error_level_t;

typedef struct {
    const char* module;
    const char* file;
    const char* source_line;
    int line;
    int column;
} error_context_t;

#ifdef __cplusplus
extern "C" {
#endif

[[noreturn]] void fatal_error(const error_context_t* ctx, const char* format, ...);
void compiler_error(const error_context_t* ctx, const char* format, ...);
void compiler_warning(const error_context_t* ctx, const char* format, ...);

[[noreturn]] void vfatal_error(const error_context_t* ctx, const char* format, va_list args);
void vcompiler_error(const error_context_t* ctx, const char* format, va_list args);
void vcompiler_warning(const error_context_t* ctx, const char* format, va_list args);

#ifdef __cplusplus
}
#endif

#endif /* ERROR_H* /
