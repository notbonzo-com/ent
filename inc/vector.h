#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <stdlib.h>

/*
 * DECLARE_VECTOR_TYPE(TYPE)
 *
 * Generates a struct and function prototypes for a dynamic array of `TYPE`.
 * Example usage:
 *     DECLARE_VECTOR_TYPE(int)
 *
 * This gives you:
 *     typedef struct { int *data; size_t size; size_t capacity; } int_vector_t;
 *     void   int_vector_init(int_vector_t* vec);
 *     void   int_vector_destroy(int_vector_t* vec);
 *     int    int_vector_reserve(int_vector_t* vec, size_t new_capacity);
 *     int    int_vector_push_back(int_vector_t* vec, int value);
 *     void   int_vector_pop_back(int_vector_t* vec);
 *     int*   int_vector_at(int_vector_t* vec, size_t index);
 *     const int* int_vector_const_at(const int_vector_t* vec, size_t index);
 *     size_t int_vector_size(const int_vector_t* vec);
 *     size_t int_vector_capacity(const int_vector_t* vec);
 */
#define DECLARE_VECTOR_TYPE(TYPE)                                               \
    typedef struct {                                                            \
        TYPE*  data;                                                            \
        size_t size;                                                            \
        size_t capacity;                                                        \
    } TYPE##_vector_t;                                                          \
                                                                                \
    void   TYPE##_vector_init(TYPE##_vector_t* vec);                            \
    void   TYPE##_vector_destroy(TYPE##_vector_t* vec);                         \
    int    TYPE##_vector_reserve(TYPE##_vector_t* vec, size_t new_capacity);    \
    int    TYPE##_vector_push_back(TYPE##_vector_t* vec, TYPE value);           \
    void   TYPE##_vector_pop_back(TYPE##_vector_t* vec);                        \
    TYPE*  TYPE##_vector_at(TYPE##_vector_t* vec, size_t index);                \
    const TYPE* TYPE##_vector_const_at(const TYPE##_vector_t* vec, size_t idx); \
    size_t TYPE##_vector_size(const TYPE##_vector_t* vec);                      \
    size_t TYPE##_vector_capacity(const TYPE##_vector_t* vec);

/*
 * DEFINE_VECTOR_FUNCTIONS(TYPE)
 *
 * Defines (implements) all the functions declared by DECLARE_VECTOR_TYPE(TYPE).
 * This should be used in the corresponding .c file and only once per vector type.
 */
#define DEFINE_VECTOR_FUNCTIONS(TYPE)                                                   \
    void TYPE##_vector_init(TYPE##_vector_t* vec) {                                     \
        vec->data     = nullptr;                                                        \
        vec->size     = 0;                                                              \
        vec->capacity = 0;                                                              \
    }                                                                                   \
                                                                                        \
    void TYPE##_vector_destroy(TYPE##_vector_t* vec) {                                  \
        if (vec->data) {                                                                \
            free(vec->data);                                                            \
            vec->data = nullptr;                                                        \
        }                                                                               \
        vec->size     = 0;                                                              \
        vec->capacity = 0;                                                              \
    }                                                                                   \
                                                                                        \
    int TYPE##_vector_reserve(TYPE##_vector_t* vec, size_t new_capacity) {              \
        if (new_capacity <= vec->capacity) {                                            \
            return 0;                                                                   \
        }                                                                               \
        TYPE* new_data = (TYPE*)realloc(vec->data, new_capacity * sizeof(TYPE));        \
        if (!new_data) {                                                                \
            return -1;                                                                  \
        }                                                                               \
        vec->data     = new_data;                                                       \
        vec->capacity = new_capacity;                                                   \
        return 0;                                                                       \
    }                                                                                   \
                                                                                        \
    int TYPE##_vector_push_back(TYPE##_vector_t* vec, TYPE value) {                     \
        if (vec->size == vec->capacity) {                                               \
            size_t new_capacity = (vec->capacity == 0) ? 1 : vec->capacity * 2;         \
            if (TYPE##_vector_reserve(vec, new_capacity) != 0) {                        \
                return -1;                                                              \
            }                                                                           \
        }                                                                               \
        vec->data[vec->size++] = value;                                                 \
        return 0;                                                                       \
    }                                                                                   \
                                                                                        \
    void TYPE##_vector_pop_back(TYPE##_vector_t* vec) {                                 \
        if (vec->size > 0) {                                                            \
            vec->size--;                                                                \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    TYPE* TYPE##_vector_at(TYPE##_vector_t* vec, size_t index) {                        \
        if (index >= vec->size) {                                                       \
            return nullptr;                                                             \
        }                                                                               \
        return &vec->data[index];                                                       \
    }                                                                                   \
                                                                                        \
    const TYPE* TYPE##_vector_const_at(const TYPE##_vector_t* vec, size_t idx){         \
        if (idx >= vec->size) {                                                         \
            return nullptr;                                                             \
        }                                                                               \
        return &vec->data[idx];                                                         \
    }                                                                                   \
                                                                                        \
    size_t TYPE##_vector_size(const TYPE##_vector_t* vec) {                             \
        return vec->size;                                                               \
    }                                                                                   \
                                                                                        \
    size_t TYPE##_vector_capacity(const TYPE##_vector_t* vec) {                         \
        return vec->capacity;                                                           \
    }

#endif /* VECTOR_H */
