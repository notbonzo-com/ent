#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static void print_error_header(const error_context_t *ctx, const char *level_color, const char *level_name)
{
    fprintf(stderr, ANSI_BOLD_WHITE "ents: %s%s:%s ", level_color, level_name, ANSI_RESET);
    if (ctx->module) {
        fprintf(stderr, "[module: %s] ", ctx->module);
    }
    if (ctx->file) {
        fprintf(stderr, "[file: %s, line: %d, column: %d] ", ctx->file, ctx->line, ctx->column);
    }
    fprintf(stderr, "\n");

    if (ctx->source_line) {
        fprintf(stderr, ANSI_BOLD_WHITE "  %s\n" ANSI_RESET, ctx->source_line);
        /* Point to the offending column, if available */
        if (ctx->column > 0 && ctx->column <= (int)strlen(ctx->source_line)) {
            fprintf(stderr, "  ");
            for (int i = 1; i < ctx->column; i++) {
                fprintf(stderr, " ");
            }
            fprintf(stderr, ANSI_BOLD_RED "^\n" ANSI_RESET);
        }
    }
}

static void print_error_message(const char *format, va_list args)
{
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
}

static void handle_error(const error_context_t *ctx, const char *level_color, const char *level_name, const char *format, va_list args, int fatal)
{
    print_error_header(ctx, level_color, level_name);
    print_error_message(format, args);

    if (fatal) {
        fprintf(stderr, ANSI_BOLD_WHITE "compilation terminated." ANSI_RESET "\n");
        exit(EXIT_FAILURE);
    }
}

[[noreturn]] void vfatal_error(const error_context_t *ctx, const char *format, va_list args)
{
    handle_error(ctx, ANSI_BOLD_RED, "fatal error", format, args, 1);
    __builtin_unreachable();
}

void vcompiler_error(const error_context_t *ctx, const char *format, va_list args)
{
    handle_error(ctx, ANSI_BOLD_RED, "error", format, args, 0);
}

void vcompiler_warning(const error_context_t *ctx, const char *format, va_list args)
{
    handle_error(ctx, ANSI_BOLD_YELLOW, "warning", format, args, 0);
}

[[noreturn]] void fatal_error(const error_context_t *ctx, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    handle_error(ctx, ANSI_BOLD_RED, "fatal error", format, args, 1);
    __builtin_unreachable();
    va_end(args);
}

void compiler_error(const error_context_t *ctx, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    handle_error(ctx, ANSI_BOLD_RED, "error", format, args, 0);
    va_end(args);
}

void compiler_warning(const error_context_t *ctx, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    handle_error(ctx, ANSI_BOLD_YELLOW, "warning", format, args, 0);
    va_end(args);
}
