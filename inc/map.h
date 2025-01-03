#ifndef MAP_H
#define MAP_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Utility macro for naming the generated functions. */
#define HASHMAP_FN(NAME, FUNC) NAME##_##FUNC

/**
 * @brief Generates the node and map struct definitions, plus function prototypes.
 *
 * @param KEY_T   The key type.
 * @param VAL_T   The value type.
 * @param NAME    A short name used to generate struct & function identifiers.
 */
#define DECLARE_HASHMAP_TYPE(KEY_T, VAL_T, NAME)                               \
    typedef struct NAME##_node_s {                                             \
        KEY_T key;                                                             \
        VAL_T value;                                                           \
        struct NAME##_node_s* next;                                            \
    } NAME##_node_t;                                                           \
                                                                               \
    typedef struct {                                                           \
        NAME##_node_t** buckets;                                               \
        size_t capacity;                                                       \
        size_t size;                                                           \
    } NAME##_t;                                                                \
                                                                               \
    void   HASHMAP_FN(NAME, init)   (NAME##_t* map, size_t capacity);          \
    void   HASHMAP_FN(NAME, destroy)(NAME##_t* map);                           \
    int    HASHMAP_FN(NAME, insert) (NAME##_t* map, KEY_T key, VAL_T value);   \
    VAL_T* HASHMAP_FN(NAME, get)    (NAME##_t* map, KEY_T key);                \
    int    HASHMAP_FN(NAME, remove) (NAME##_t* map, KEY_T key);                \
    size_t HASHMAP_FN(NAME, size)   (const NAME##_t* map);                     \
    size_t HASHMAP_FN(NAME, capacity)(const NAME##_t* map);

/**
 * @brief Defines (implements) the functions for a previously declared hashmap.
 *
 * @param KEY_T     The key type.
 * @param VAL_T     The value type.
 * @param NAME      Matches the name used in DECLARE_HASHMAP_TYPE.
 * @param HASH_FN   A function pointer for hashing a (const KEY_T*).
 * @param CMP_FN    A function pointer for comparing two (const KEY_T*).
 * @param KEY_PTR   1 if the key is dynamically allocated (and should be freed),
 *                  0 otherwise.
 * @param VAL_PTR   1 if the value is dynamically allocated (and should be freed),
 *                  0 otherwise.
 */
#define DEFINE_HASHMAP_FUNCTIONS(KEY_T, VAL_T, NAME, HASH_FN, CMP_FN, KEY_PTR, VAL_PTR)   \
    void HASHMAP_FN(NAME, init)(NAME##_t* map, size_t capacity) {                         \
        if (capacity == 0) {                                                              \
            capacity = 8;                                                                 \
        }                                                                                 \
        map->buckets = (NAME##_node_t**)calloc(capacity, sizeof(NAME##_node_t*));         \
        map->capacity = capacity;                                                         \
        map->size = 0;                                                                    \
    }                                                                                     \
                                                                                          \
    void HASHMAP_FN(NAME, destroy)(NAME##_t* map) {                                       \
        if (!map->buckets) return;                                                        \
        for (size_t i = 0; i < map->capacity; ++i) {                                      \
            NAME##_node_t* node = map->buckets[i];                                        \
            while (node) {                                                                \
                NAME##_node_t* tmp = node;                                                \
                node = node->next;                                                        \
                if (KEY_PTR) {                                                            \
                    free((void*)tmp->key);                                                \
                }                                                                         \
                if (VAL_PTR) {                                                            \
                    free((void*)tmp->value);                                              \
                }                                                                         \
                free(tmp);                                                                \
            }                                                                             \
        }                                                                                 \
        free(map->buckets);                                                               \
        map->buckets = NULL;                                                              \
        map->capacity = 0;                                                                \
        map->size = 0;                                                                    \
    }                                                                                     \
                                                                                          \
    int HASHMAP_FN(NAME, insert)(NAME##_t* map, KEY_T key, VAL_T value) {                 \
        if (!map->buckets) return -1;                                                     \
        uint64_t hval = HASH_FN(&key);                                                    \
        size_t idx = hval % map->capacity;                                                \
        NAME##_node_t* curr = map->buckets[idx];                                          \
        while (curr) {                                                                    \
            if (CMP_FN(&curr->key, &key) == 0) {                                          \
                if (VAL_PTR) {                                                            \
                    free((void*)curr->value);                                             \
                }                                                                         \
                curr->value = value;                                                      \
                return 0;                                                                 \
            }                                                                             \
            curr = curr->next;                                                            \
        }                                                                                 \
        NAME##_node_t* new_node = (NAME##_node_t*)malloc(sizeof(NAME##_node_t));          \
        if (!new_node) return -1;                                                         \
        new_node->key = key;                                                              \
        new_node->value = value;                                                          \
        new_node->next = map->buckets[idx];                                               \
        map->buckets[idx] = new_node;                                                     \
        map->size++;                                                                      \
        return 0;                                                                         \
    }                                                                                     \
                                                                                          \
    VAL_T* HASHMAP_FN(NAME, get)(NAME##_t* map, KEY_T key) {                              \
        if (!map->buckets) return NULL;                                                   \
        uint64_t hval = HASH_FN(&key);                                                    \
        size_t idx = hval % map->capacity;                                                \
        NAME##_node_t* curr = map->buckets[idx];                                          \
        while (curr) {                                                                    \
            if (CMP_FN(&curr->key, &key) == 0) {                                          \
                return &curr->value;                                                      \
            }                                                                             \
            curr = curr->next;                                                            \
        }                                                                                 \
        return NULL;                                                                      \
    }                                                                                     \
                                                                                          \
    int HASHMAP_FN(NAME, remove)(NAME##_t* map, KEY_T key) {                              \
        if (!map->buckets) return -1;                                                     \
        uint64_t hval = HASH_FN(&key);                                                    \
        size_t idx = hval % map->capacity;                                                \
        NAME##_node_t* curr = map->buckets[idx];                                          \
        NAME##_node_t* prev = NULL;                                                       \
        while (curr) {                                                                    \
            if (CMP_FN(&curr->key, &key) == 0) {                                          \
                if (prev) {                                                               \
                    prev->next = curr->next;                                              \
                } else {                                                                  \
                    map->buckets[idx] = curr->next;                                       \
                }                                                                         \
                if (KEY_PTR) {                                                            \
                    free((void*)curr->key);                                               \
                }                                                                         \
                if (VAL_PTR) {                                                            \
                    free((void*)curr->value);                                             \
                }                                                                         \
                free(curr);                                                               \
                map->size--;                                                              \
                return 0;                                                                 \
            }                                                                             \
            prev = curr;                                                                  \
            curr = curr->next;                                                            \
        }                                                                                 \
        return -1;                                                                        \
    }                                                                                     \
                                                                                          \
    size_t HASHMAP_FN(NAME, size)(const NAME##_t* map) {                                  \
        return map->size;                                                                 \
    }                                                                                     \
                                                                                          \
    size_t HASHMAP_FN(NAME, capacity)(const NAME##_t* map) {                              \
        return map->capacity;                                                             \
    }


#endif /* MAP_H */