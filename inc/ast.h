#ifndef AST_H
#define AST_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

enum ast_code {
    /* Top-level */
    AST_CODE_TRANSLATION_UNIT,
    AST_CODE_LIST,

    /* Declarations */
    AST_CODE_FUNCTION_DECL,
    AST_CODE_VAR_DECL,
    AST_CODE_PARAM_DECL,
    AST_CODE_TYPEDEF_DECL,
    AST_CODE_STRUCT_DECL,
    AST_CODE_UNION_DECL,
    AST_CODE_ENUM_DECL,
    AST_CODE_FIELD_DECL,
    AST_CODE_ENUM_CONST_DECL,

    /* Definitions */
    AST_CODE_FUNCTION_DEF,

    /* Statements */
    AST_CODE_COMPOUND_STMT,
    AST_CODE_IF_STMT,
    AST_CODE_SWITCH_STMT,
    AST_CODE_CASE_LABEL_STMT,
    AST_CODE_DEFAULT_LABEL_STMT,
    AST_CODE_WHILE_STMT,
    AST_CODE_TIMES_STMT,
    AST_CODE_FOR_STMT,
    AST_CODE_BREAK_STMT,
    AST_CODE_CONTINUE_STMT,
    AST_CODE_RETURN_STMT,
    AST_CODE_ASM_STMT,        /* inline assembly */
    AST_CODE_NULL_STMT,       /* empty ';' */

    /* Expressions */
    AST_CODE_IDENTIFIER,
    AST_CODE_INTEGER_LITERAL,
    AST_CODE_FLOAT_LITERAL,
    AST_CODE_CHAR_LITERAL,
    AST_CODE_STRING_LITERAL,
    AST_CODE_UNARY_EXPR,
    AST_CODE_BINARY_EXPR,
    AST_CODE_CONDITIONAL_EXPR,  /* ternary operator ?: */
    AST_CODE_CALL_EXPR,
    AST_CODE_MEMBER_EXPR,       /* struct/union member (arrow), UFCS is translated on parse-time into AST_CODE_CALL_EXPR */
    AST_CODE_ARRAY_SUBSCRIPT_EXPR,
    AST_CODE_CAST_EXPR,
    AST_CODE_COMPOUND_LITERAL_EXPR,
    AST_CODE_SIZEOF_EXPR,
    AST_CODE_ALIGNOF_EXPR,

    /* Type references */
    AST_CODE_TYPE_NAME,
    AST_CODE_ATTRIBUTE, /* inline, static, extern, etc for everything else */
    AST_CODE_TYPE_SPECIFIER, /* e.g., byte, single, etc. */

};

struct ast_common {
    enum ast_code code;
    const char* filename; /* Source filename should be a shared pointer to not waste memory! */
    int line;
    int column;
};

struct ast_node_list {
    struct ast_common common;
    size_t length;
    struct ast_common* *elements;
};


/* ---------------------------------------------------------------------------
 * Top-Level
 * --------------------------------------------------------------------------- */
struct ast_translation_unit {
    struct ast_common common;
    struct ast_node_list* decls;
};

/* ---------------------------------------------------------------------------
 * Declarations
 * --------------------------------------------------------------------------- */
struct ast_function_decl {
    struct ast_common common;
    struct ast_common* return_type; /* ast_type_specifier or type_name node */
    const char* name;
    struct ast_node_list* params; /* list of ast_param_decl */
    struct ast_node_list* attributes;
};

struct ast_var_decl {
    struct ast_common common;
    struct ast_common* type_spec; /* pointer to type-related node(s) */
    struct ast_common* init_value; /* optional initializer (expression), or nullptr */
    const char* name; /* identifier name */
    struct ast_node_list* attributes;
};

struct ast_param_decl {
    struct ast_common common;
    struct ast_common* type_spec;
    const char* name; /* May be nullptr */
    struct ast_node_list* attributes; /* only really for const */
    bool is_variadic; /* is it ... */
};

struct ast_typedef_decl {
    struct ast_common common;
    struct ast_common* type_spec;
    const char* alias_name;
};

struct ast_struct_decl {
    struct ast_common common;
    const char* tag_name;  /* struct tag: "s" in `struct s` */
    struct ast_node_list* fields; /* list of fields (ast_field_decl) */
    bool is_complete; /* indicates if the struct is fully defined or just forward-declared */
    struct ast_node_list* attributes;
};

struct ast_union_decl {
    struct ast_common common;
    const char* tag_name;
    struct ast_node_list* fields;
    bool is_complete;
    struct ast_node_list* attributes;
};

struct ast_enum_decl {
    struct ast_common common;
    const char* tag_name;
    struct ast_node_list* consts; /* list of enum consts (ast_enum_const_decl) */
    bool is_complete;
    struct ast_node_list* attributes;
};

struct ast_field_decl {
    struct ast_common common;
    struct ast_common* type_spec;
    const char* name;
    bool is_bitfield;
    int bit_width;
    struct ast_node_list* attributes;
};

struct ast_enum_const_decl {
    struct ast_common common;
    const char       * name;
    struct ast_common* value_expr; /* optional init expression for enumerator, should be a compile-time evaluatable expr */
};


/* ---------------------------------------------------------------------------
 * Definitions
 * --------------------------------------------------------------------------- */
struct ast_function_def {
    struct ast_common common;
    struct ast_common* return_type; /* ast_type_specifier or type_name node */
    const char* name;
    struct ast_node_list* params; /* list of ast_param_decl */
    struct ast_common* body; /* ast_compound_stmt */
    struct ast_node_list* attributes;
};

/* ---------------------------------------------------------------------------
 * Statements
 * --------------------------------------------------------------------------- */
struct ast_compound_stmt {
    struct ast_common common;
    struct ast_node_list* stmts;
};

struct ast_if_stmt {
    struct ast_common common;
    struct ast_common* cond_expr;
    struct ast_common* then_body;
    struct ast_common* else_body;
};

struct ast_switch_stmt {
    struct ast_common common;
    struct ast_common* cond_expr;
    struct ast_common* body;  /* ast_compound_stmt with case labels */
};

struct ast_case_label_stmt {
    struct ast_common common;
    struct ast_common* value_expr; /* expression for the case label */
};

struct ast_default_label_stmt {
    struct ast_common common;
};

struct ast_while_stmt {
    struct ast_common common;
    struct ast_common* cond_expr;
    struct ast_common* body;
};

struct ast_times_stmt {
    struct ast_common common;
    struct ast_common* n_value; /* how many times to loop */
    struct ast_common* body;
    bool show_index;
    struct ast_common* index_type; /* nullptr if show_index is false */
    struct ast_common* index_name; /* nullptr if show_index is false */
};

struct ast_for_stmt {
    struct ast_common common;
    struct ast_common* init; /* can be a declaration or expression */
    struct ast_common* cond_expr;
    struct ast_common* incr_expr;
    struct ast_common* body;
};

struct ast_break_stmt {
    struct ast_common common;
};

struct ast_continue_stmt {
    struct ast_common common;
};

struct ast_return_stmt {
    struct ast_common common;
    struct ast_common* return_expr; /* nullptr for `return;` in void function */
};

struct ast_asm_stmt {
    struct ast_common common;
    const char* assembly_code;
    /* todo, asm attributes and coresponding stuff */
};

struct ast_null_stmt {
    struct ast_common common;
};

/* ---------------------------------------------------------------------------
 * Expressions
 * --------------------------------------------------------------------------- */
struct ast_identifier {
    struct ast_common common;
    const char* name;
};

struct ast_integer_literal {
    struct ast_common common;
    long long value;   /* store integer constant */
    bool is_unsigned;
    int bits;
};

struct ast_float_literal {
    struct ast_common common;
    double value;
    /* TODO could store more info about float type like single vs double, etc. */
};

struct ast_char_literal {
    struct ast_common common;
#if 0
    char value[2];  /* two bytes to allow for storing escape sequences */
#endif
    int value; /* store the character code */
};

struct ast_string_literal {
    struct ast_common common;
    const char* value;  /* pointer to the string contents */
};

enum ast_unop_kind {
    AST_UNOP_POSTINC,
    AST_UNOP_POSTDEC,
    AST_UNOP_PREINC,
    AST_UNOP_PREDEC,
    AST_UNOP_ADDR_OF,  /* &x */
    AST_UNOP_DEREF,    /** x */
    AST_UNOP_PLUS,     /* +x */
    AST_UNOP_MINUS,    /* -x */
    AST_UNOP_NOT,      /* !x */
    AST_UNOP_COMP,     /* ~x (bitwise complement) */ /* TODO Maybe remove? */
};

struct ast_unary_expr {
    struct ast_common common;
    enum ast_unop_kind op;
    struct ast_common* operand;
};

enum ast_binop_kind {
    /* Arithmetic */
    AST_BINOP_ADD,
    AST_BINOP_SUB,
    AST_BINOP_MUL,
    AST_BINOP_DIV,
    AST_BINOP_MOD,

    /* Bitwise */
    AST_BINOP_SHL,
    AST_BINOP_SHR,
    AST_BINOP_AND,
    AST_BINOP_OR,
    AST_BINOP_XOR,

    /* Logical */
    AST_BINOP_LOGICAL_AND,
    AST_BINOP_LOGICAL_OR,

    /* Comparison */
    AST_BINOP_EQ,
    AST_BINOP_NE,
    AST_BINOP_LT,
    AST_BINOP_GT,
    AST_BINOP_LE,
    AST_BINOP_GE,

    /* Assignment variants: =, +=, -=, etc */
    AST_BINOP_ASSIGN,
    AST_BINOP_ADD_ASSIGN,
    AST_BINOP_SUB_ASSIGN,
    AST_BINOP_MUL_ASSIGN,
    AST_BINOP_DIV_ASSIGN,
    AST_BINOP_MOD_ASSIGN,
    AST_BINOP_SHL_ASSIGN,
    AST_BINOP_SHR_ASSIGN,
    AST_BINOP_AND_ASSIGN,
    AST_BINOP_XOR_ASSIGN,
    AST_BINOP_OR_ASSIGN
};

struct ast_binary_expr {
    struct ast_common common;
    enum ast_binop_kind op;
    struct ast_common* lhs;
    struct ast_common* rhs;
};

/* Conditional (ternary) expression: `cond ? expr1 : expr2` */
struct ast_conditional_expr {
    struct ast_common common;
    struct ast_common* cond;
    struct ast_common* then_expr;
    struct ast_common* else_expr;
};

/* we don't care if the call originated from UFCS, its syntactical sugar */
struct ast_call_expr {
    struct ast_common common;
    struct ast_common* callee; /* expression for the function being called */
    struct ast_node_list* arguments; /* list of expressions */
};

struct ast_member_expr {
    struct ast_common common;
    struct ast_common* base_expr;
    const char* member_name;
};

/* Array subscript: `arr[idx]` */
struct ast_array_subscript_expr {
    struct ast_common common;
    struct ast_common* array_expr;
    struct ast_common* index_expr;
};

struct ast_cast_expr {
    struct ast_common common;
    struct ast_common* type_node;
    struct ast_common* expr;
};

/* Compound literal expression: `(int[]){1,2,3}` or more usefully (struct gdt_descriptor){0,0,0,0,0} */
struct ast_compound_literal_expr {
    struct ast_common common;
    struct ast_common* type_node;
    struct ast_common* init_list; /* ast_node_list of initializers */
};

struct ast_sizeof_expr {
    struct ast_common common;
    bool is_expr;      /* indicates if `type_or_expr` is an expression */
    struct ast_common* type_or_expr; /* can be a type node or an expr node */
};

struct ast_alignof_expr {
    struct ast_common common;
    bool is_expr;      /* indicates if `type_or_expr` is an expression */
    struct ast_common* type_or_expr; /* can be a type node or an expr node */
};

/* ---------------------------------------------------------------------------
 * Type references
 * --------------------------------------------------------------------------- */
struct ast_type_name {
    struct ast_common common;
    /* TODO a real compiler would break this down further like storing pointer to declarations, qualifiers, etc. */
    struct ast_common* type_spec;
    struct ast_node_list* attributes;
};

enum ast_builtin_type_kind {
    AST_BUILTIN_VOID,
    AST_BUILTIN_BYTE,
    AST_BUILTIN_WORD,
    AST_BUILTIN_DWORD,
    AST_BUILTIN_QWORD,
    AST_BUILTIN_SBYTE,
    AST_BUILTIN_SWORD,
    AST_BUILTIN_SDWORD,
    AST_BUILTIN_SQWORD,
    AST_BUILTIN_SINGLE,
    AST_BUILTIN_DOUBLE,
    AST_BUILTIN_VAARGS,
};

struct ast_type_specifier {
    struct ast_common common;
    enum ast_builtin_type_kind builtin_kind;
    /* TODO could also point to struct/union/enum if it's that kind. */
};

/* ---------------------------------------------------------------------------
 * Attributes
 * --------------------------------------------------------------------------- */
struct ast_attribute {
    struct ast_common common;
    const char* attr_name;
};

enum ast_attribute_names_kind {
    AST_ATTRIBUTE_NULL = 0,
    AST_ATTRIBUTE_CONST,
    AST_ATTRIBUTE_VOLATILE,
    AST_ATTRIBUTE_STATIC,
    AST_ATTRIBUTE_INLINE,
    AST_ATTRIBUTE_PACKED,
    AST_ATTRIBUTE_ALIGNED,
    AST_ATTRIBUTE_NORETURN,
    AST_ATTRIBUTE_DEPRECATED,
    AST_ATTRIBUTE_COUNT
};
extern char* ast_attribute_names[AST_ATTRIBUTE_COUNT];

/* ---------------------------------------------------------------------------
 * Functions
 * --------------------------------------------------------------------------- */

/* --- Generic node creation/destroy --- */
struct ast_common* ast_create_node(enum ast_code code);
void ast_destroy_node(struct ast_common* node);

/* --- Node list utilities --- */
extern struct ast_node_list* ast_create_node_list(void);
extern void ast_destroy_node_list(struct ast_node_list* list);
extern void ast_node_list_append(struct ast_node_list* list, struct ast_common* element);

/* ---------------------------------------------------------------------------
 * Macros
 * --------------------------------------------------------------------------- */
#define AST_IS_TYPE(NODE, CODE) ((NODE) && ((NODE)->code == (CODE)))
#define AST_CAST_TYPE(NODE, CODE, TYPE) (AST_IS_TYPE((NODE), (CODE)) ? (TYPE* )(NODE) : nullptr)

#define AST_IS_FUNCTION_DECL(NODE) AST_IS_TYPE((NODE), AST_CODE_FUNCTION_DECL)
#define AST_AS_FUNCTION_DECL(NODE) ((struct ast_function_decl* )(NODE))

#define AST_IS_FUNCTION_DEF(NODE)  AST_IS_TYPE((NODE), AST_CODE_FUNCTION_DEF)
#define AST_AS_FUNCTION_DEF(NODE)  ((struct ast_function_def* )(NODE))

#define AST_IS_PARAM_DECL(NODE)    AST_IS_TYPE((NODE), AST_CODE_PARAM_DECL)
#define AST_AS_PARAM_DECL(NODE)    ((struct ast_param_decl* )(NODE))

#define AST_IS_TYPEDEF_DECL(NODE)  AST_IS_TYPE((NODE), AST_CODE_TYPEDEF_DECL)
#define AST_AS_TYPEDEF_DECL(NODE)  ((struct ast_typedef_decl* )(NODE))

#define AST_IS_STRUCT_DECL(NODE)   AST_IS_TYPE((NODE), AST_CODE_STRUCT_DECL)
#define AST_AS_STRUCT_DECL(NODE)   ((struct ast_struct_decl* )(NODE))

#define AST_IS_UNION_DECL(NODE)    AST_IS_TYPE((NODE), AST_CODE_UNION_DECL)
#define AST_AS_UNION_DECL(NODE)    ((struct ast_union_decl* )(NODE))

#define AST_IS_ENUM_DECL(NODE)     AST_IS_TYPE((NODE), AST_CODE_ENUM_DECL)
#define AST_AS_ENUM_DECL(NODE)     ((struct ast_enum_decl* )(NODE))

#define AST_IS_FIELD_DECL(NODE)    AST_IS_TYPE((NODE), AST_CODE_FIELD_DECL)
#define AST_AS_FIELD_DECL(NODE)    ((struct ast_field_decl* )(NODE))

#define AST_IS_ENUM_CONST_DECL(NODE) AST_IS_TYPE((NODE), AST_CODE_ENUM_CONST_DECL)
#define AST_AS_ENUM_CONST_DECL(NODE) ((struct ast_enum_const_decl* )(NODE))

#define AST_IS_COMPOUND_STMT(NODE) AST_IS_TYPE((NODE), AST_CODE_COMPOUND_STMT)
#define AST_AS_COMPOUND_STMT(NODE) ((struct ast_compound_stmt* )(NODE))

#define AST_IS_IF_STMT(NODE)       AST_IS_TYPE((NODE), AST_CODE_IF_STMT)
#define AST_AS_IF_STMT(NODE)       ((struct ast_if_stmt* )(NODE))

#define AST_IS_SWITCH_STMT(NODE)   AST_IS_TYPE((NODE), AST_CODE_SWITCH_STMT)
#define AST_AS_SWITCH_STMT(NODE)   ((struct ast_switch_stmt* )(NODE))

#define AST_IS_CASE_LABEL_STMT(NODE)    AST_IS_TYPE((NODE), AST_CODE_CASE_LABEL_STMT)
#define AST_AS_CASE_LABEL_STMT(NODE)    ((struct ast_case_label_stmt* )(NODE))

#define AST_IS_DEFAULT_LABEL_STMT(NODE) AST_IS_TYPE((NODE), AST_CODE_DEFAULT_LABEL_STMT)
#define AST_AS_DEFAULT_LABEL_STMT(NODE) ((struct ast_default_label_stmt* )(NODE))

#define AST_IS_WHILE_STMT(NODE)    AST_IS_TYPE((NODE), AST_CODE_WHILE_STMT)
#define AST_AS_WHILE_STMT(NODE)    ((struct ast_while_stmt* )(NODE))

#define AST_IS_TIMES_STMT(NODE)    AST_IS_TYPE((NODE), AST_CODE_TIMES_STMT)
#define AST_AS_TIMES_STMT(NODE)    ((struct ast_times_stmt* )(NODE))

#define AST_IS_FOR_STMT(NODE)      AST_IS_TYPE((NODE), AST_CODE_FOR_STMT)
#define AST_AS_FOR_STMT(NODE)      ((struct ast_for_stmt* )(NODE))

#define AST_IS_BREAK_STMT(NODE)    AST_IS_TYPE((NODE), AST_CODE_BREAK_STMT)
#define AST_AS_BREAK_STMT(NODE)    ((struct ast_break_stmt* )(NODE))

#define AST_IS_CONTINUE_STMT(NODE) AST_IS_TYPE((NODE), AST_CODE_CONTINUE_STMT)
#define AST_AS_CONTINUE_STMT(NODE) ((struct ast_continue_stmt* )(NODE))

#define AST_IS_RETURN_STMT(NODE)   AST_IS_TYPE((NODE), AST_CODE_RETURN_STMT)
#define AST_AS_RETURN_STMT(NODE)   ((struct ast_return_stmt* )(NODE))

#define AST_IS_ASM_STMT(NODE)      AST_IS_TYPE((NODE), AST_CODE_ASM_STMT)
#define AST_AS_ASM_STMT(NODE)      ((struct ast_asm_stmt* )(NODE))

#define AST_IS_NULL_STMT(NODE)     AST_IS_TYPE((NODE), AST_CODE_NULL_STMT)
#define AST_AS_NULL_STMT(NODE)     ((struct ast_null_stmt* )(NODE))

#define AST_IS_IDENTIFIER(NODE)    AST_IS_TYPE((NODE), AST_CODE_IDENTIFIER)
#define AST_AS_IDENTIFIER(NODE)    ((struct ast_identifier* )(NODE))

#define AST_IS_INTEGER_LITERAL(NODE) AST_IS_TYPE((NODE), AST_CODE_INTEGER_LITERAL)
#define AST_AS_INTEGER_LITERAL(NODE) ((struct ast_integer_literal* )(NODE))

#define AST_IS_FLOAT_LITERAL(NODE) AST_IS_TYPE((NODE), AST_CODE_FLOAT_LITERAL)
#define AST_AS_FLOAT_LITERAL(NODE) ((struct ast_float_literal* )(NODE))

#define AST_IS_CHAR_LITERAL(NODE)  AST_IS_TYPE((NODE), AST_CODE_CHAR_LITERAL)
#define AST_AS_CHAR_LITERAL(NODE)  ((struct ast_char_literal* )(NODE))

#define AST_IS_STRING_LITERAL(NODE) AST_IS_TYPE((NODE), AST_CODE_STRING_LITERAL)
#define AST_AS_STRING_LITERAL(NODE) ((struct ast_string_literal* )(NODE))

#define AST_IS_UNARY_EXPR(NODE)    AST_IS_TYPE((NODE), AST_CODE_UNARY_EXPR)
#define AST_AS_UNARY_EXPR(NODE)    ((struct ast_unary_expr* )(NODE))

#define AST_IS_BINARY_EXPR(NODE)   AST_IS_TYPE((NODE), AST_CODE_BINARY_EXPR)
#define AST_AS_BINARY_EXPR(NODE)   ((struct ast_binary_expr* )(NODE))

#define AST_IS_CONDITIONAL_EXPR(NODE) AST_IS_TYPE((NODE), AST_CODE_CONDITIONAL_EXPR)
#define AST_AS_CONDITIONAL_EXPR(NODE) ((struct ast_conditional_expr* )(NODE))

#define AST_IS_CALL_EXPR(NODE)     AST_IS_TYPE((NODE), AST_CODE_CALL_EXPR)
#define AST_AS_CALL_EXPR(NODE)     ((struct ast_call_expr* )(NODE))

#define AST_IS_MEMBER_EXPR(NODE)   AST_IS_TYPE((NODE), AST_CODE_MEMBER_EXPR)
#define AST_AS_MEMBER_EXPR(NODE)   ((struct ast_member_expr* )(NODE))

#define AST_IS_ARRAY_SUBSCRIPT_EXPR(NODE) AST_IS_TYPE((NODE), AST_CODE_ARRAY_SUBSCRIPT_EXPR)
#define AST_AS_ARRAY_SUBSCRIPT_EXPR(NODE) ((struct ast_array_subscript_expr* )(NODE))

#define AST_IS_CAST_EXPR(NODE)     AST_IS_TYPE((NODE), AST_CODE_CAST_EXPR)
#define AST_AS_CAST_EXPR(NODE)     ((struct ast_cast_expr* )(NODE))

#define AST_IS_COMPOUND_LITERAL_EXPR(NODE) AST_IS_TYPE((NODE), AST_CODE_COMPOUND_LITERAL_EXPR)
#define AST_AS_COMPOUND_LITERAL_EXPR(NODE) ((struct ast_compound_literal_expr* )(NODE))

#define AST_IS_SIZEOF_EXPR(NODE)   AST_IS_TYPE((NODE), AST_CODE_SIZEOF_EXPR)
#define AST_AS_SIZEOF_EXPR(NODE)   ((struct ast_sizeof_expr* )(NODE))

#define AST_IS_ALIGNOF_EXPR(NODE)  AST_IS_TYPE((NODE), AST_CODE_ALIGNOF_EXPR)
#define AST_AS_ALIGNOF_EXPR(NODE)  ((struct ast_alignof_expr* )(NODE))

#define AST_IS_TYPE_NAME(NODE)     AST_IS_TYPE((NODE), AST_CODE_TYPE_NAME)
#define AST_AS_TYPE_NAME(NODE)     ((struct ast_type_name* )(NODE))

#define AST_IS_TYPE_SPECIFIER(NODE) AST_IS_TYPE((NODE), AST_CODE_TYPE_SPECIFIER)
#define AST_AS_TYPE_SPECIFIER(NODE) ((struct ast_type_specifier* )(NODE))

#define AST_IS_ATTRIBUTE(NODE)     AST_IS_TYPE((NODE), AST_CODE_ATTRIBUTE)
#define AST_AS_ATTRIBUTE(NODE)     ((struct ast_attribute* )(NODE))

#define AST_IS_STATEMENT(NODE) \
    ((NODE) && \
     ( ((NODE)->code >= AST_CODE_COMPOUND_STMT && (NODE)->code <= AST_CODE_NULL_STMT) || \
       (NODE)->code == AST_CODE_CASE_LABEL_STMT || (NODE)->code == AST_CODE_DEFAULT_LABEL_STMT ))

#define AST_IS_ANY_STATEMENT(NODE) \
    ( AST_IS_COMPOUND_STMT(NODE) || AST_IS_IF_STMT(NODE) || AST_IS_SWITCH_STMT(NODE) || \
      AST_IS_CASE_LABEL_STMT(NODE) || AST_IS_DEFAULT_LABEL_STMT(NODE) || AST_IS_WHILE_STMT(NODE) || \
      AST_IS_TIMES_STMT(NODE) || AST_IS_FOR_STMT(NODE) || AST_IS_BREAK_STMT(NODE) || \
      AST_IS_CONTINUE_STMT(NODE) || AST_IS_RETURN_STMT(NODE) || AST_IS_ASM_STMT(NODE) || \
      AST_IS_NULL_STMT(NODE) )

#define AST_IS_DECLARATION(NODE) \
    ( AST_IS_FUNCTION_DECL(NODE) || AST_IS_VAR_DECL(NODE) || AST_IS_PARAM_DECL(NODE) || \
      AST_IS_TYPEDEF_DECL(NODE)  || AST_IS_STRUCT_DECL(NODE) || AST_IS_UNION_DECL(NODE) || \
      AST_IS_ENUM_DECL(NODE)     || AST_IS_FIELD_DECL(NODE) || AST_IS_ENUM_CONST_DECL(NODE) )

#define AST_IS_EXPRESSION(NODE) \
    ( AST_IS_IDENTIFIER(NODE) || AST_IS_INTEGER_LITERAL(NODE) || AST_IS_FLOAT_LITERAL(NODE) || \
      AST_IS_CHAR_LITERAL(NODE) || AST_IS_STRING_LITERAL(NODE) || AST_IS_UNARY_EXPR(NODE) || \
      AST_IS_BINARY_EXPR(NODE)  || AST_IS_CONDITIONAL_EXPR(NODE) || AST_IS_CALL_EXPR(NODE) || \
      AST_IS_MEMBER_EXPR(NODE)  || AST_IS_ARRAY_SUBSCRIPT_EXPR(NODE) || AST_IS_CAST_EXPR(NODE) || \
      AST_IS_COMPOUND_LITERAL_EXPR(NODE) || AST_IS_SIZEOF_EXPR(NODE) || AST_IS_ALIGNOF_EXPR(NODE) )

/* Check if a given binop is an assignment binop */
#define AST_BINOP_IS_ASSIGN(OP) \
    ((OP) == AST_BINOP_ASSIGN      || (OP) == AST_BINOP_ADD_ASSIGN || \
     (OP) == AST_BINOP_SUB_ASSIGN  || (OP) == AST_BINOP_MUL_ASSIGN || \
     (OP) == AST_BINOP_DIV_ASSIGN  || (OP) == AST_BINOP_MOD_ASSIGN || \
     (OP) == AST_BINOP_SHL_ASSIGN  || (OP) == AST_BINOP_SHR_ASSIGN || \
     (OP) == AST_BINOP_AND_ASSIGN  || (OP) == AST_BINOP_XOR_ASSIGN || \
     (OP) == AST_BINOP_OR_ASSIGN)

/* Check if a given binop is a “pure” arithmetic binop (no assignment) */
#define AST_BINOP_IS_ARITH(OP) \
    ((OP) == AST_BINOP_ADD || (OP) == AST_BINOP_SUB || \
     (OP) == AST_BINOP_MUL || (OP) == AST_BINOP_DIV || \
     (OP) == AST_BINOP_MOD)

/* Check if a given unop is a prefix or postfix increment/decrement */
#define AST_UNOP_IS_INCREMENT_OR_DECREMENT(OP) \
    ((OP) == AST_UNOP_PREINC  || (OP) == AST_UNOP_PREDEC  || \
     (OP) == AST_UNOP_POSTINC || (OP) == AST_UNOP_POSTDEC)

#define AST_NODE_FILENAME(NODE) ((NODE) && (NODE)->filename ? (NODE)->filename : "(unknown)")

#define AST_NODE_LINE(NODE)   ((NODE) ? (NODE)->line : -1)
#define AST_NODE_COLUMN(NODE) ((NODE) ? (NODE)->column : -1)

#define AST_CHECK_AND_CAST_TYPE(NODE, IS_MACRO, STRUCT_TYPE) ((STRUCT_TYPE* )( (IS_MACRO(NODE)) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_TRANSLATION_UNIT(NODE) \
((struct ast_translation_unit* )( ( (NODE) && (NODE)->code == AST_CODE_TRANSLATION_UNIT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_NODE_LIST(NODE) \
((struct ast_node_list* )( ( (NODE) && (NODE)->code == AST_CODE_LIST ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_FUNCTION_DECL(NODE) \
((struct ast_function_decl* )( ( (NODE) && (NODE)->code == AST_CODE_FUNCTION_DECL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_VAR_DECL(NODE) \
((struct ast_var_decl* )( ( (NODE) && (NODE)->code == AST_CODE_VAR_DECL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_PARAM_DECL(NODE) \
((struct ast_param_decl* )( ( (NODE) && (NODE)->code == AST_CODE_PARAM_DECL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_TYPEDEF_DECL(NODE) \
((struct ast_typedef_decl* )( ( (NODE) && (NODE)->code == AST_CODE_TYPEDEF_DECL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_STRUCT_DECL(NODE) \
((struct ast_struct_decl* )( ( (NODE) && (NODE)->code == AST_CODE_STRUCT_DECL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_UNION_DECL(NODE) \
((struct ast_union_decl* )( ( (NODE) && (NODE)->code == AST_CODE_UNION_DECL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_ENUM_DECL(NODE) \
((struct ast_enum_decl* )( ( (NODE) && (NODE)->code == AST_CODE_ENUM_DECL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_FIELD_DECL(NODE) \
((struct ast_field_decl* )( ( (NODE) && (NODE)->code == AST_CODE_FIELD_DECL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_ENUM_CONST_DECL(NODE) \
((struct ast_enum_const_decl* )( ( (NODE) && (NODE)->code == AST_CODE_ENUM_CONST_DECL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_FUNCTION_DEF(NODE) \
((struct ast_function_def* )( ( (NODE) && (NODE)->code == AST_CODE_FUNCTION_DEF ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_COMPOUND_STMT(NODE) \
((struct ast_compound_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_COMPOUND_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_IF_STMT(NODE) \
((struct ast_if_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_IF_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_SWITCH_STMT(NODE) \
((struct ast_switch_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_SWITCH_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_CASE_LABEL_STMT(NODE) \
((struct ast_case_label_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_CASE_LABEL_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_DEFAULT_LABEL_STMT(NODE) \
((struct ast_default_label_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_DEFAULT_LABEL_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_WHILE_STMT(NODE) \
((struct ast_while_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_WHILE_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_TIMES_STMT(NODE) \
((struct ast_times_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_TIMES_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_FOR_STMT(NODE) \
((struct ast_for_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_FOR_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_BREAK_STMT(NODE) \
((struct ast_break_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_BREAK_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_CONTINUE_STMT(NODE) \
((struct ast_continue_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_CONTINUE_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_RETURN_STMT(NODE) \
((struct ast_return_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_RETURN_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_ASM_STMT(NODE) \
((struct ast_asm_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_ASM_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_NULL_STMT(NODE) \
((struct ast_null_stmt* )( ( (NODE) && (NODE)->code == AST_CODE_NULL_STMT ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_IDENTIFIER(NODE) \
((struct ast_identifier* )( ( (NODE) && (NODE)->code == AST_CODE_IDENTIFIER ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_INTEGER_LITERAL(NODE) \
((struct ast_integer_literal* )( ( (NODE) && (NODE)->code == AST_CODE_INTEGER_LITERAL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_FLOAT_LITERAL(NODE) \
((struct ast_float_literal* )( ( (NODE) && (NODE)->code == AST_CODE_FLOAT_LITERAL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_CHAR_LITERAL(NODE) \
((struct ast_char_literal* )( ( (NODE) && (NODE)->code == AST_CODE_CHAR_LITERAL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_STRING_LITERAL(NODE) \
((struct ast_string_literal* )( ( (NODE) && (NODE)->code == AST_CODE_STRING_LITERAL ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_UNARY_EXPR(NODE) \
((struct ast_unary_expr* )( ( (NODE) && (NODE)->code == AST_CODE_UNARY_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_BINARY_EXPR(NODE) \
((struct ast_binary_expr* )( ( (NODE) && (NODE)->code == AST_CODE_BINARY_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_CONDITIONAL_EXPR(NODE) \
((struct ast_conditional_expr* )( ( (NODE) && (NODE)->code == AST_CODE_CONDITIONAL_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_CALL_EXPR(NODE) \
((struct ast_call_expr* )( ( (NODE) && (NODE)->code == AST_CODE_CALL_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_MEMBER_EXPR(NODE) \
((struct ast_member_expr* )( ( (NODE) && (NODE)->code == AST_CODE_MEMBER_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_ARRAY_SUBSCRIPT_EXPR(NODE) \
((struct ast_array_subscript_expr* )( ( (NODE) && (NODE)->code == AST_CODE_ARRAY_SUBSCRIPT_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_CAST_EXPR(NODE) \
((struct ast_cast_expr* )( ( (NODE) && (NODE)->code == AST_CODE_CAST_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_COMPOUND_LITERAL_EXPR(NODE) \
((struct ast_compound_literal_expr* )( ( (NODE) && (NODE)->code == AST_CODE_COMPOUND_LITERAL_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_SIZEOF_EXPR(NODE) \
((struct ast_sizeof_expr* )( ( (NODE) && (NODE)->code == AST_CODE_SIZEOF_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_ALIGNOF_EXPR(NODE) \
((struct ast_alignof_expr* )( ( (NODE) && (NODE)->code == AST_CODE_ALIGNOF_EXPR ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_TYPE_NAME(NODE) \
((struct ast_type_name* )( ( (NODE) && (NODE)->code == AST_CODE_TYPE_NAME ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_TYPE_SPECIFIER(NODE) \
((struct ast_type_specifier* )( ( (NODE) && (NODE)->code == AST_CODE_TYPE_SPECIFIER ) ? (NODE) : nullptr ))

#define AST_CHECK_AND_CAST_ATTRIBUTE(NODE) \
((struct ast_attribute* )( ( (NODE) && (NODE)->code == AST_CODE_ATTRIBUTE ) ? (NODE) : nullptr ))

#define AST_CREATE_ATTR(kind)                                                                                                       \
        ((struct ast_attribute) {                                                                                                   \
            .common = {                                                                                                             \
                .code = AST_CODE_ATTRIBUTE,                                                                                         \
                .filename = nullptr,                                                                                                   \
                .line = 0,                                                                                                          \
                .column = 0                                                                                                         \
            },                                                                                                                      \
            .attr_name = ((kind) >= 0 && (kind) < AST_ATTRIBUTE_COUNT) ? ast_attribute_names[(kind)] : ast_attribute_names[(0)]     \
        })

#endif /* AST_H */
