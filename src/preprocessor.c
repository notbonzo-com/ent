#include "lexer.h"
#include <preprocessor.h>
#include <error.h>
#include <safemem.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>

#if __has_feature(cxx_constexpr)
    constexpr char HEADER_REGEX[] = "^\\s*header\\s*\\{";
    constexpr char DEFINE_REGEX[] = "^\\s*define\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s+(.*)$";
    constexpr char INCLUDE_REGEX[] = "^\\s*include\\s*[\"<](.*)[\">]\\s*";
#else
    #define HEADER_REGEX "^\\s*header\\s*\\{"
    #define DEFINE_REGEX "^\\s*define\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s+(.*)$"
    #define INCLUDE_REGEX "^\\s*include\\s*[\"<](.*)[\">]\\s*"
#endif

#define REGCHECK_OR_FATAL(_pp, _regex_ptr, _pattern, _cflags)                            \
    do {                                                                                 \
        int __ret = regcomp((_regex_ptr), (_pattern), (_cflags));                        \
        if (__ret) {                                                                     \
            char __errbuf[256];                                                          \
            regerror(__ret, (_regex_ptr), __errbuf, sizeof(__errbuf));                   \
            error_context_t ctx = make_error_context((_pp), __FILE__, __LINE__);         \
            fatal_error(&ctx, "Regex error for '%s': %s", _pattern, __errbuf);           \
        }                                                                                \
    } while (0)


static ssize_t _getline(char **lineptr, size_t *n, FILE *stream);
static error_context_t make_error_context(const struct preprocessor *pp, const char *file, int line);
static void process_line(struct preprocessor *pp, const char *line, regex_t *header_start_regex, regex_t *define_regex, regex_t *include_regex);
static void process_line_in_header(struct preprocessor *pp, const char *line, regex_t *header_start_regex, regex_t *define_regex, regex_t *include_regex);
static void handle_include(struct preprocessor *pp, const char *line, regex_t *include_regex);
static void handle_header_start(struct preprocessor *pp, const char *line);
static void handle_define(struct preprocessor *pp, const char *line, regex_t *define_regex);
static void merge_defines(struct preprocessor *dest, const struct preprocessor *src);

static void append_string(char **dest, const char *src);
static int count_braces(const char *line);

/* --------------------------------------------------------------------------
 * Helper functions
 * -------------------------------------------------------------------------- */

static void trim_leading_whitespace(char *str)
{
    if (!str) return;
    char *p = str;
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    if (p != str) {
        size_t len = strlen(p);
        memmove(str, p, len + 1);
    }
}

/* --------------------------------------------------------------------------
 * Conditional stack management
 * -------------------------------------------------------------------------- */

static void conditional_stack_push(struct preprocessor *pp, bool in_true_block) {
    if (pp->conditional_stack_size == pp->conditional_stack_capacity) {
        pp->conditional_stack_capacity = pp->conditional_stack_capacity
                                         ? pp->conditional_stack_capacity * 2
                                         : 8;
        pp->conditional_stack = realloc(
            pp->conditional_stack,
            sizeof(struct preprocessor_conditional_state) * pp->conditional_stack_capacity
        );
        if (!pp->conditional_stack) {
            error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
            fatal_error(&ctx, "Out of memory expanding conditional stack");
        }
    }
    
    pp->conditional_stack[pp->conditional_stack_size++] = (struct preprocessor_conditional_state){
        .in_true_block = in_true_block,
        .condition_met = false, /* set to true later if met */
    };
}

static void conditional_stack_pop(struct preprocessor *pp) {
    if (pp->conditional_stack_size == 0) {
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Mismatched @endif encountered");
    }
    pp->conditional_stack_size--;
}

static struct preprocessor_conditional_state *conditional_stack_top(struct preprocessor *pp) {
    if (pp->conditional_stack_size == 0) return nullptr;
    return &pp->conditional_stack[pp->conditional_stack_size - 1];
}


/* --------------------------------------------------------------------------
 * Condition evaluation
 * -------------------------------------------------------------------------- */

static bool evaluate_condition(struct preprocessor *pp, const char *condition)
{
    char symbol[256] = {0};
    char operator_str[4] = {0};
    char operand[256] = {0};

    int parsed = sscanf(condition, "%255s %3s %255s", symbol, operator_str, operand);
    if (parsed != 3) { /* TODO: Add columns to point to the part where the parsing failed. */
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Malformed condition: '%s'. Expected format: SYMBOL <op> VALUE", condition);
    }

    char *defined_value = nullptr;
    for (size_t i = 0; i < pp->define_count; i++) {
        if (strcmp(pp->defines[i].name, symbol) == 0) {
            defined_value = pp->defines[i].value;
            break;
        }
    }

    if (!defined_value) {
        return false;
    }

    bool both_numeric = true;
    for (const char *p = defined_value; *p; p++) {
        if ((*p < '0' || *p > '9') && *p != '-') { 
            both_numeric = false;
            break;
        }
    }
    for (const char *p = operand; *p; p++) {
        if ((*p < '0' || *p > '9') && *p != '-') {
            both_numeric = false;
            break;
        }
    }

    if (both_numeric) {
        long val_def  = strtol(defined_value, nullptr, 10);
        long val_cond = strtol(operand,       nullptr, 10);

        if (strcmp(operator_str, "==") == 0) {
            return (val_def == val_cond);
        } else if (strcmp(operator_str, "!=") == 0) {
            return (val_def != val_cond);
        } else if (strcmp(operator_str, ">") == 0) {
            return (val_def > val_cond);
        } else if (strcmp(operator_str, ">=") == 0) {
            return (val_def >= val_cond);
        } else if (strcmp(operator_str, "<") == 0) {
            return (val_def < val_cond);
        } else if (strcmp(operator_str, "<=") == 0) {
            return (val_def <= val_cond);
        } else {
            error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
            fatal_error(&ctx, "Unsupported operator: '%s'", operator_str);
            __builtin_unreachable();
        }
    } 
    else {
        int cmp = strcmp(defined_value, operand);

        if (strcmp(operator_str, "==") == 0) {
            return (cmp == 0);
        } else if (strcmp(operator_str, "!=") == 0) {
            return (cmp != 0);
        } else if (strcmp(operator_str, ">") == 0) {
            return (cmp > 0);
        } else if (strcmp(operator_str, ">=") == 0) {
            return (cmp >= 0);
        } else if (strcmp(operator_str, "<") == 0) {
            return (cmp < 0);
        } else if (strcmp(operator_str, "<=") == 0) {
            return (cmp <= 0);
        } else {
            error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
            fatal_error(&ctx, "Unsupported operator: '%s' for string comparison", operator_str);
            __builtin_unreachable();
        }
    }
}

/* Can add more conditionals later */
static bool handle_conditional_directive(struct preprocessor *pp, const char *line)
{
    while (*line == ' ') line++;

    if (strncmp(line, "@ifdef", 6) == 0 &&
        (line[6] == ' ' || line[6] == '\0')) 
    {
        const char *symbol = line + 6;
        while (*symbol == ' ') symbol++;

        bool defined = false;
        for (size_t i = 0; i < pp->define_count; i++) {
            if (strcmp(pp->defines[i].name, symbol) == 0) {
                defined = true;
                break;
            }
        }

        conditional_stack_push(pp, defined);

        struct preprocessor_conditional_state *st = conditional_stack_top(pp);
        st->condition_met = defined;

        return true;
    }

    if (strncmp(line, "@if", 3) == 0 &&
        (line[3] == ' ' || line[3] == '\0')) 
    {
        const char *condition = line + 3;
        while (*condition == ' ') condition++;

        bool condition_result = evaluate_condition(pp, condition);

        conditional_stack_push(pp, condition_result);

        struct preprocessor_conditional_state *st = conditional_stack_top(pp);
        st->condition_met = condition_result;

        return true;
    }

    if (strncmp(line, "@elif", 5) == 0 &&
        (line[5] == ' ' || line[5] == '\0')) 
    {
        const char *condition = line + 5;
        while (*condition == ' ') condition++;

        struct preprocessor_conditional_state *state = conditional_stack_top(pp);
        if (!state) {
            error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
            fatal_error(&ctx, "Mismatched @elif encountered without a prior @if/@ifdef");
        }

        if (state->condition_met) {
            /* A previous branch was already taken => skip this branch */
            state->in_true_block = false;
        } else {
            bool condition_result = evaluate_condition(pp, condition);

            state->in_true_block = condition_result;
            state->condition_met = condition_result;
        }
        return true;
    }

    if (strncmp(line, "@else", 5) == 0 &&
        (line[5] == ' ' || line[5] == '\0' || line[5] == '\n' || line[5] == '\r'))
    {
        struct preprocessor_conditional_state *state = conditional_stack_top(pp);
        if (!state) {
            error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
            fatal_error(&ctx, "Mismatched @else encountered without a prior @if/@ifdef");
        }

        /* NO prior branch is met */
        state->in_true_block = !state->condition_met;
        state->condition_met = true;

        return true;
    }

    if (strncmp(line, "@endif", 6) == 0 &&
        (line[6] == ' ' || line[6] == '\0' || line[6] == '\n' || line[6] == '\r'))
    {
        if (pp->conditional_stack_size == 0) {
            error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
            fatal_error(&ctx, "Mismatched @endif encountered");
        }
        conditional_stack_pop(pp);
        return true;
    }

    return false;
}


/* --------------------------------------------------------------------------
 * Preprocessor API interface
 * -------------------------------------------------------------------------- */

struct preprocessor preprocessor_create(const char *filename)
{
    struct preprocessor pp;
    memset(&pp, 0, sizeof(pp));

    pp.filename        = filename;
    pp.in_header_block = false;
    pp.brace_balance   = 0;
    pp.current_line    = 0;
    pp.current_column  = 0;
    token_t_vector_init(&pp.tokens);

    pp.file = fopen(filename, "r");
    if (!pp.file) {
        error_context_t ctx = make_error_context(&pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Cannot open file '%s'", filename);
    }

    regex_t header_start_regex;
    regex_t define_regex;
    regex_t include_regex;

    REGCHECK_OR_FATAL(&pp, &header_start_regex, HEADER_REGEX,  REG_EXTENDED);
    REGCHECK_OR_FATAL(&pp, &define_regex,       DEFINE_REGEX,  REG_EXTENDED | REG_NEWLINE);
    REGCHECK_OR_FATAL(&pp, &include_regex,      INCLUDE_REGEX, REG_EXTENDED);

    char   *linebuf  = nullptr;
    size_t  bufsize  = 0;
    ssize_t linelen  = 0;

    while ((linelen = _getline(&linebuf, &bufsize, pp.file)) != -1) {
        pp.current_line++;
        pp.current_column = 1;

        if (linelen > 0 && linebuf[linelen - 1] == '\n') {
            linebuf[linelen - 1] = '\0';
        }

        /* Keep the raw line in pp->line for error messages */
        SAFE_FREE(pp.line);
        pp.line = strdup(linebuf);

        process_line(&pp, linebuf, &header_start_regex, &define_regex, &include_regex);
    }

    /* If we ended but still have an unclosed header block, error out */
    if (pp.in_header_block && pp.brace_balance != 0) {
        error_context_t ctx = make_error_context(&pp, __FILE__, __LINE__);
        fatal_error(&ctx,
                    "Unclosed header block in '%s', brace_balance=%d",
                    filename,
                    pp.brace_balance);
    }

    regfree(&header_start_regex);
    regfree(&define_regex);
    regfree(&include_regex);
    SAFE_FREE(linebuf);

    fclose(pp.file);
    pp.file = nullptr;

    struct lexer lex;
    lexer_init(&lex, pp.preprocessed_file, pp.filename);
    lexer_add_tokens_to_vector(&pp.tokens, &lex.tokens);
    lexer_destroy(&lex);

    return pp;
}

void preprocessor_destroy(struct preprocessor *pp)
{
    if (!pp) return;

    if (pp->file) {
        fclose(pp->file);
        pp->file = nullptr;
    }

    SAFE_FREE(pp->preprocessed_file);
    pp->preprocessed_file = nullptr;

    SAFE_FREE(pp->header_content);
    pp->header_content = nullptr;

    SAFE_FREE(pp->line);
    pp->line = nullptr;

    if (pp->includes) {
        for (size_t i = 0; i < pp->include_count; i++) {
            SAFE_FREE(pp->includes[i]);
        }
        SAFE_FREE(pp->includes);
        pp->includes = nullptr;
    }

    if (pp->defines) {
        for (size_t i = 0; i < pp->define_count; i++) {
            SAFE_FREE(pp->defines[i].name);
            SAFE_FREE(pp->defines[i].value);
        }
        SAFE_FREE(pp->defines);
        pp->defines = nullptr;
    }

    SAFE_FREE(pp->conditional_stack);
    pp->conditional_stack = nullptr;
}

static void process_line(struct preprocessor *pp,
                         const char *line,
                         regex_t *header_start_regex,
                         regex_t *define_regex,
                         regex_t *include_regex)
{
    pp->current_column = 1;
    
    /* If we're in a header block, defer to the header-based function */
    if (pp->in_header_block) {
        process_line_in_header(pp, line, header_start_regex, define_regex, include_regex);
        return;
    }

    /* 1) conditional directive? */
    if (handle_conditional_directive(pp, line)) {
        return; 
    }

    /* 2) inactive block? */
    struct preprocessor_conditional_state *state = conditional_stack_top(pp);
    if (state && !state->in_true_block) {
        return;
    }

    /* regex shaite */
    if (regexec(header_start_regex, line, 0, nullptr, 0) == 0) {
        handle_header_start(pp, line);
        return;
    }
    if (regexec(define_regex, line, 0, nullptr, 0) == 0) {
        handle_define(pp, line, define_regex);
        return;
    }
    if (regexec(include_regex, line, 0, nullptr, 0) == 0) {
        handle_include(pp, line, include_regex);
        return;
    }

    /* normal text, append */
    append_string(&pp->preprocessed_file, line);
    append_string(&pp->preprocessed_file, "\n");
}

static void process_line_in_header(struct preprocessor *pp,
                                   const char *line,
                                   regex_t *header_start_regex,
                                   regex_t *define_regex,
                                   regex_t *include_regex)
{
    /* conditional directive? */
    if (handle_conditional_directive(pp, line)) {
        return;
    }

    /* inactive block? */
    struct preprocessor_conditional_state *state = conditional_stack_top(pp);
    if (state && !state->in_true_block) {
        return;
    }

    /* defines or includes inside header */
    if (regexec(header_start_regex, line, 0, nullptr, 0) == 0) {
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Nested header block in '%s'", pp->filename);
        __builtin_unreachable();
    }
    if (regexec(define_regex, line, 0, nullptr, 0) == 0) {
        handle_define(pp, line, define_regex);
        return; 
    }
    if (regexec(include_regex, line, 0, nullptr, 0) == 0) {
        handle_include(pp, line, include_regex);
        return;
    }

    int line_braces = count_braces(line);
    pp->brace_balance += line_braces;

    if (pp->brace_balance <= 0) {
        /*
         * We found the brace that closes this header block.
         * The line might have text preceding the '}' that still
         * belongs to the block, so let's capture that portion.
         */
        int    local_balance = line_braces;
        size_t search_pos    = 0;
        size_t line_len      = strlen(line);
        char  *processed_line = nullptr;

        while (local_balance < 0 && search_pos < line_len) {
            char *brace_ptr = strchr(&line[search_pos], '}');
            if (!brace_ptr) break;
            local_balance++;
            if (local_balance == 0) {
                size_t length = (size_t)(brace_ptr - line);
                processed_line = calloc(length + 1, sizeof(char));
                strncpy(processed_line, line, length);
                break;
            }
            search_pos = (size_t)(brace_ptr - line) + 1;
        }

        if (processed_line) {
            /* Up to that '}' is still part of the header content */
            append_string(&pp->header_content, processed_line);
            append_string(&pp->header_content, "\n");

            /* Also add it to the final preprocessed output */
            append_string(&pp->preprocessed_file, processed_line);
            append_string(&pp->preprocessed_file, "\n");
            SAFE_FREE(processed_line);
        }
        pp->in_header_block = false;
    } 
    else {
        append_string(&pp->header_content, line);
        append_string(&pp->header_content, "\n");

        append_string(&pp->preprocessed_file, line);
        append_string(&pp->preprocessed_file, "\n");
    }
}

static void handle_include(struct preprocessor *pp,
                           const char *line,
                           regex_t *include_regex)
{
    regmatch_t matches[2];
    if (regexec(include_regex, line, 2, matches, 0) != 0) {
        pp->current_column = 1; /* TODO Point exactly to the thing that failed */
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Bad include syntax: '%s'", line);
    }

    pp->current_column = (int)(matches[0].rm_so + 1);

    regoff_t start = matches[1].rm_so;
    regoff_t end   = matches[1].rm_eo;
    size_t length  = (size_t)(end - start);

    char *include_path = calloc(length + 1, sizeof(char));
    strncpy(include_path, &line[start], length);

    /* Check for include cycle */
    for (size_t i = 0; i < pp->include_count; i++) {
        if (strcmp(pp->includes[i], include_path) == 0) {
            error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
            fatal_error(&ctx, "Cyclic include: '%s'", include_path);
        }
    }

    /* Expand the includes array */
    pp->includes = realloc(pp->includes, sizeof(char*) * (pp->include_count + 1));
    if (!pp->includes) {
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Out of memory while expanding includes array");
    }
    pp->includes[pp->include_count] = include_path;
    pp->include_count++;

    struct preprocessor sub = preprocessor_create(include_path);
    merge_defines(pp, &sub);

    struct lexer sub_lexer;
    lexer_init(&sub_lexer, sub.header_content, sub.filename);
    lexer_add_tokens_to_vector(&pp->tokens, &sub_lexer.tokens);
    lexer_destroy(&sub_lexer);

    preprocessor_destroy(&sub);
}

static void handle_header_start(struct preprocessor *pp, const char *line)
{
    pp->in_header_block = true;

    SAFE_FREE(pp->header_content);
    pp->header_content = nullptr;

    pp->brace_balance = 0;

    int line_braces = count_braces(line);
    pp->brace_balance += line_braces;

    /* find the '{' */
    const char *brace_pos = strchr(line, '{');
    if (!brace_pos) {
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Malformed header line, missing '{': '%s'", line);
    }
    const char *after_brace = brace_pos + 1;

    /* If we opened and closed on the same line */
    if (pp->brace_balance == 0) {
        /* extract content between '{' and '}' */
        const char *closing_brace = strrchr(after_brace, '}');
        if (closing_brace) {
            size_t content_len = (size_t)(closing_brace - after_brace);
            if (content_len > 0) {
                char *content = calloc(content_len + 1, sizeof(char));
                strncpy(content, after_brace, content_len);

                append_string(&pp->header_content, content);
                append_string(&pp->header_content, "\n");

                append_string(&pp->preprocessed_file, content);
                append_string(&pp->preprocessed_file, "\n");
                SAFE_FREE(content);
            }
        }
        pp->in_header_block = false;
    } 
    else {
        append_string(&pp->header_content, after_brace);
        append_string(&pp->header_content, "\n");

        append_string(&pp->preprocessed_file, after_brace);
        append_string(&pp->preprocessed_file, "\n");
    }
}

static void handle_define(struct preprocessor *pp,
                          const char *line,
                          regex_t *define_regex)
{
    regmatch_t matches[3];
    /* capturing group #1 => name, group #2 => value */
    int ret = regexec(define_regex, line, 3, matches, 0);
    if (ret != 0) {
        pp->current_column = 1; /* TODO Point exactly to the thing that failed */
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Malformed define statement: '%s'", line);
    }

    pp->current_column = (int)(matches[0].rm_so + 1);

    regoff_t start_name  = matches[1].rm_so;
    regoff_t end_name    = matches[1].rm_eo;
    regoff_t start_value = matches[2].rm_so;
    regoff_t end_value   = matches[2].rm_eo;

    size_t name_len  = (size_t)(end_name   - start_name);
    size_t value_len = (size_t)(end_value  - start_value);

    /* never happens because of regex, maybe we can fix it later? TODO */
    /*if (name_len == 0) {
        pp->current_column = (int)(matches[1].rm_so);
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Missing name in define: '%s'", line);
    }*/
    if (value_len == 0) {
        pp->current_column = (int)(matches[2].rm_so);
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Missing value in define: '%s'", line);
    }

    char *name  = calloc(name_len + 1,  sizeof(char));
    char *value = calloc(value_len + 1, sizeof(char));
    strncpy(name,  &line[start_name],  name_len);
    strncpy(value, &line[start_value], value_len);

    for (size_t i = 0; i < pp->define_count; i++) {
        if (strcmp(pp->defines[i].name, name) == 0) {
            pp->current_column = (int)(matches[1].rm_so + 1);
            error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
            fatal_error(&ctx, "Macro '%s' is already defined", name);
        }
    }

    pp->defines = realloc(pp->defines, sizeof(struct preprocessor_define) * (pp->define_count + 1));
    if (!pp->defines) { /* Wrong check, TODO fix */
        pp->current_column = (int)(matches[0].rm_so + 1);
        error_context_t ctx = make_error_context(pp, __FILE__, __LINE__);
        fatal_error(&ctx, "Out of memory while storing define");
    }
    pp->defines[pp->define_count].name  = name;
    pp->defines[pp->define_count].value = value;
    pp->define_count++;
}

static void merge_defines(struct preprocessor *dest, const struct preprocessor *src)
{
    for (size_t i = 0; i < src->define_count; i++) {
        const char *sub_name  = src->defines[i].name;
        const char *sub_value = src->defines[i].value;

        for (size_t j = 0; j < dest->define_count; j++) {
            if (strcmp(dest->defines[j].name, sub_name) == 0) {
                dest->current_column = strlen(sub_name);
                error_context_t ctx = make_error_context(dest, __FILE__, __LINE__);
                fatal_error(&ctx, "Symbol '%s' already defined.", sub_name);
                __builtin_unreachable();
            }
        }

        size_t name_len  = strlen(sub_name);
        size_t value_len = strlen(sub_value);

        char *name_cpy  = calloc(name_len + 1, sizeof(char));
        char *value_cpy = calloc(value_len + 1, sizeof(char));

        strcpy(name_cpy,  sub_name);
        strcpy(value_cpy, sub_value);

        dest->defines = realloc(dest->defines,
                                sizeof(struct preprocessor_define) * (dest->define_count + 1));
        if (!dest->defines) {
            error_context_t ctx = make_error_context(dest, __FILE__, __LINE__);
            fatal_error(&ctx, "Out of memory while merging defines");
        }
        dest->defines[dest->define_count].name  = name_cpy;
        dest->defines[dest->define_count].value = value_cpy;
        dest->define_count++;
    }
}

static void append_string(char **dest, const char *src)
{
    if (!src) return;
    if (!*dest) {
        size_t len = strlen(src);
        *dest = calloc(len + 1, sizeof(char));
        strcpy(*dest, src);
    } else {
        size_t old_len = strlen(*dest);
        size_t src_len = strlen(src);
        char  *temp    = realloc(*dest, old_len + src_len + 1);
        if (!temp) {
            SAFE_FREE(*dest);
            *dest = nullptr;
            return;
        }
        *dest = temp;
        strcpy((*dest) + old_len, src);
    }
}

static int count_braces(const char *line)
{
    int balance = 0;
    for (const char *p = line; *p; p++) {
        if (*p == '{') balance++;
        if (*p == '}') balance--;
    }
    return balance;
}

static ssize_t _getline(char **lineptr, size_t *n, FILE *stream)
{
    if (!lineptr || !n || !stream) return -1;

    size_t size = (*n > 0) ? *n : 128;
    char  *buf  = *lineptr ? *lineptr : malloc(size);
    if (!buf) return -1;

    size_t pos = 0;
    int c;
    while ((c = fgetc(stream)) != EOF) {
        if (pos + 1 >= size) {
            size_t new_size = size * 2;
            char *temp = realloc(buf, new_size);
            if (!temp) {
                SAFE_FREE(buf);
                return -1;
            }
            buf  = temp;
            size = new_size;
        }
        buf[pos++] = (char)c;
        if (c == '\n') break; 
    }

    if (pos == 0 && c == EOF) {
        SAFE_FREE(buf);
        *lineptr = nullptr;
        return -1;
    }
    buf[pos] = '\0';

    *lineptr = buf;
    *n       = size;

    trim_leading_whitespace(*lineptr);

    return (ssize_t)pos;
}

static error_context_t make_error_context(const struct preprocessor *pp, 
                                          const char *file, 
                                          int line)
{
    error_context_t ctx;
    ctx.module      = "preprocessor";
    ctx.file        = (pp && pp->filename) ? pp->filename : "unknown";
    ctx.source_line = (pp && pp->line) ? pp->line : nullptr;
    ctx.line        = (int)((pp) ? pp->current_line : 0);
    ctx.column      = (int)((pp) ? pp->current_column : 0);

#ifdef DEBUG
    printf(" --- Debug Info: file=%s, line=%d, column=%d ---\n", file, line, ctx.column);
#else
    (void)file;
    (void)line;
#endif

    return ctx;
}
