#include <ast.h>
#include <safemem.h>

char* ast_attribute_names[AST_ATTRIBUTE_COUNT] = {
    "nullptr", "const", "volatile", "static", "inline", "packed", "aligned", "noreturn", "deprecated"
};

struct ast_common* ast_create_node(const enum ast_code code)
{
    struct ast_common* node = malloc(sizeof(struct ast_common));
    if (!node)
        return nullptr;

    node->code = code;
    node->filename = nullptr;
    node->line = 0;
    node->column = 0;

    return node;
}

void ast_destroy_node(struct ast_common* node)
{
    if (!node)
        return;

    SAFE_FREE(node);
}

struct ast_node_list* ast_create_node_list(void)
{
    struct ast_node_list* list = malloc(sizeof(struct ast_node_list));
    if (!list)
        return nullptr;

    list->common.code = AST_CODE_LIST;
    list->common.filename = nullptr;
    list->common.line = 0;
    list->common.column = 0;

    list->length = 0;
    list->elements = nullptr;

    return list;
}

void ast_destroy_node_list(struct ast_node_list* list)
{
    if (!list)
        return;

   for (size_t i = 0; i < list->length; i++) {
       ast_destroy_node(list->elements[i]);
   }

    if (list->elements) {
        SAFE_FREE(list->elements);
    }

    SAFE_FREE(list);
}

void ast_node_list_append(struct ast_node_list* list, struct ast_common* element)
{
    if (!list)
        return;

    const size_t old_length = list->length;
    const size_t new_length = old_length + 1;
    struct ast_common* *new_array = realloc(list->elements, sizeof(struct ast_common* ) * new_length);

    if (!new_array)
        return;

    list->elements = new_array;

    list->elements[old_length] = element;
    list->length = new_length;
}
