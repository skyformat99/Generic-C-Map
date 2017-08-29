#ifndef _MAP_H_
#define _MAP_H_

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    MAP_SUCCESS = 0,
    MAP_INPUT_OUT_OF_RANGE,
    MAP_ALLOC_FAILURE,
    MAP_KEY_NOT_FOUND
} MAP_STATUS;

#define map_elem(KEY_TYPE, VALUE_TYPE)                                         \
    struct{KEY_TYPE _key; VALUE_TYPE _value; size_t _hash; bool _in_use;}

#define map(KEY_TYPE, VALUE_TYPE)                                              \
struct                                                                         \
{                                                                              \
    MAP_STATUS status;                                                         \
    unsigned _bits; /* lg of the length of _table array */                     \
    size_t _nelem;  /* number of key/value pairs currently in the table */     \
    map_elem(KEY_TYPE, VALUE_TYPE)* _table;                                    \
    map_elem(KEY_TYPE, VALUE_TYPE) _tmp;                                       \
    size_t (*_hash_f)(void*); /* hashes a key of type KEY_TYPE to an index */  \
    bool (*_key_eq_f)(void*, void*); /* returns true if two keys are equal */  \
}

#define map_init(/* map(KEY_TYPE, VALUE_TYPE)* */p_map,                        \
    /* size_t (*)(void*) */hash_f, /* int (*)(void*, void*) */key_eq_f,        \
    /* unsigned */num_bits)                                                    \
                                  _map_init((p_map), hash_f, key_eq_f, num_bits)

#define map_deinit(/* map(KEY_TYPE, VALUE_TYPE)* */p_map)                      \
                                                            _map_deinit((p_map))

#define map_set(/* map(KEY_TYPE, VALUE_TYPE)* */p_map, /* KEY_TYPE */key,      \
    /* VALUE_TYPE */value)                                                     \
                                                   _map_set((p_map), key, value)

#define map_get(/* VALUE_TYPE* */p_ans, /* map(KEY_TYPE, VALUE_TYPE)* */p_map, \
    /* KEY_TYPE */key)                                                         \
                                                 _map_get((p_ans), (p_map), key)

#define map_key_exists(/* bool* */p_ans, /* map(KEY_TYPE, VALUE_TYPE)* */p_map,\
    /*KEY_TYPE */key)                                                          \
                                          _map_key_exists((p_ans), (p_map), key)

#define map_remove(/* map(KEY_TYPE, VALUE_TYPE)* */p_map, /* KEY_TYPE */key)   \
                                                       _map_remove((p_map), key)

#define map_length(/* size_t* */p_ans, /* map(KEY_TYPE, VALUE_TYPE)* */p_map)  \
                                                   _map_length((p_ans), (p_map))

#define map_load_factor(/* double* */p_ans,                                    \
    /* map(KEY_TYPE, VALUE_TYPE)* */p_map)                                     \
                                              _map_load_factor((p_ans), (p_map))

static size_t str_hash(void* key);
static bool str_eq(void* str1, void* str2);

///////////////////////////////// DEFINITIONS //////////////////////////////////
#define MAP_DEFAULT_BITS 16
#define MAP_BITS_PER_SIZE_T (sizeof(size_t)*CHAR_BIT)

static inline size_t _map_pow2(unsigned x)
{
    return (size_t)(1 << x);
}

static inline size_t _map_dib(size_t hash, size_t curr, unsigned table_bits)
{
    return (curr - hash) % _map_pow2(table_bits);
}

static inline void _map_memswap(void* p1, void* p2, size_t sz)
{
    char tmp, *a = p1, *b = p2;
    for (size_t i = 0; i < sz; ++i)
    {
        tmp = a[i];
        a[i] = b[i];
        b[i] = tmp;
    }
}

#define _map_init(p_map, hash_f, key_eq_f, num_bits)                           \
do                                                                             \
{                                                                              \
    p_map->_bits = num_bits;                                                   \
    if (p_map->_bits > MAP_BITS_PER_SIZE_T)                                    \
    {                                                                          \
        p_map->status = MAP_INPUT_OUT_OF_RANGE;                                \
        break;                                                                 \
    }                                                                          \
    p_map->_nelem = 0;                                                         \
    p_map->_hash_f = hash_f;                                                   \
    p_map->_key_eq_f = key_eq_f;                                               \
    p_map->_table =                                                            \
        calloc(_map_pow2(p_map->_bits), sizeof(p_map->_tmp));                  \
    if (p_map->_table)                                                         \
        p_map->status = MAP_SUCCESS;                                           \
    else                                                                       \
        p_map->status = MAP_ALLOC_FAILURE;                                     \
}while(0)

#define _map_deinit(p_map)                                                     \
do                                                                             \
{                                                                              \
    free(p_map->_table);                                                       \
    p_map->_table = NULL;                                                      \
    p_map->status = MAP_SUCCESS;                                               \
}while(0);

#define _map_set(p_map, key, value)                                            \
do                                                                             \
{                                                                              \
    /* prepare element to be set */                                            \
    p_map->_tmp._key = key;                                                    \
    p_map->_tmp._value = value;                                                \
    p_map->_tmp._hash = (p_map->_hash_f)(&(p_map->_tmp._key));                 \
    p_map->_tmp._in_use = true;                                                \
    /* insert using robin hood insertion */                                    \
    size_t __table_len = _map_pow2(p_map->_bits);                              \
    size_t __curr = p_map->_tmp._hash % __table_len;                           \
    size_t __key_not_found = true;                                             \
    while (__key_not_found && (p_map->_table)[__curr]._in_use)                 \
    {                                                                          \
        if (p_map->_key_eq_f(&((p_map->_table)[__curr]._key),                  \
            &(p_map->_tmp._key)))                                              \
        {                                                                      \
            /* key exists in the table already, so change it's value */        \
            memcpy(&((p_map->_table)[__curr]), &(p_map->_tmp),                 \
                sizeof(p_map->_tmp));                                          \
            p_map->status = MAP_SUCCESS;                                       \
            __key_not_found = false;                                           \
            break;                                                             \
        }                                                                      \
        if (_map_dib(p_map->_tmp._hash, __curr, p_map->_bits) >                \
            _map_dib((p_map->_table)[__curr]._hash, __curr, p_map->_bits))     \
        {                                                                      \
            _map_memswap(&(p_map->_tmp), &((p_map->_table)[__curr]),           \
                sizeof(p_map->_tmp));                                          \
        }                                                                      \
        __curr = (__curr + 1) % __table_len;                                   \
    }                                                                          \
    if (__key_not_found)                                                       \
    {                                                                          \
        /* insert element into first empty bucket */                           \
        memcpy(&((p_map->_table)[__curr]), &(p_map->_tmp),                     \
            sizeof(p_map->_tmp));                                              \
        /* adjust map metadata */                                              \
        ++(p_map->_nelem);                                                     \
        p_map->status = MAP_SUCCESS;                                           \
    }                                                                          \
}while(0)

#define _map_get(p_ans, p_map, key)                                            \
do                                                                             \
{                                                                              \
    bool __key_exists;                                                         \
    _map_key_exists(&__key_exists, p_map, key);                                \
    if (__key_exists)                                                          \
    {                                                                          \
        /* _map_key_exists stores the target value, so extract it */           \
        *p_ans = p_map->_tmp._value;                                           \
        p_map->status = MAP_SUCCESS;                                           \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        p_map->status = MAP_KEY_NOT_FOUND;                                     \
    }                                                                          \
}while(0)

#define _map_key_exists(p_ans, p_map, key)                                     \
do                                                                             \
{                                                                              \
    p_map->_tmp._key = key;                                                    \
    p_map->_tmp._hash = (p_map->_hash_f)(&(p_map->_tmp._key));                 \
    size_t __table_len = _map_pow2(p_map->_bits);                              \
    size_t __curr = p_map->_tmp._hash % __table_len;                           \
    *p_ans = false;                                                            \
    while ((p_map->_table)[__curr]._in_use &&                                  \
        _map_dib(p_map->_tmp._hash, __curr, p_map->_bits) >=                   \
        _map_dib((p_map->_table)[__curr]._hash, __curr, p_map->_bits))         \
    {                                                                          \
        if (p_map->_key_eq_f(&((p_map->_table)[__curr]._key),                  \
            &(p_map->_tmp._key)))                                              \
        {                                                                      \
            *p_ans = true;                                                     \
            /* map_get needs the value at __curr */                            \
            p_map->_tmp._value = (p_map->_table)[__curr]._value;               \
            /* map_remove needs the location of the element (i.e. __curr) */   \
             /* use _hash as tmp index holder, not an actual hash */           \
            p_map->_tmp._hash = __curr;                                        \
            break;                                                             \
        }                                                                      \
        __curr = (__curr + 1) % __table_len;                                   \
    }                                                                          \
    p_map->status = MAP_SUCCESS;                                               \
}while(0)

#define _map_remove(p_map, key)                                                \
do                                                                             \
{                                                                              \
    bool __key_exists;                                                         \
    _map_key_exists(&__key_exists, p_map, key);                                \
    if (__key_exists)                                                          \
    {                                                                          \
        /* _map_key_exists stores the index where target element was found */  \
        /* Note: _tmp._hash isn't actually a hash. It's the index where the */ \
        /* target element actually ended up. */                                \
        size_t __target_pos = p_map->_tmp._hash;                               \
        size_t __stop_pos = __target_pos + 1;                                  \
        size_t __table_len = _map_pow2(p_map->_bits);                          \
        while ((p_map->_table)[__stop_pos]._in_use &&                          \
            _map_dib(__target_pos, __stop_pos, p_map->_bits) > 0)              \
        {                                                                      \
            __stop_pos = (__stop_pos + 1) % __table_len;                       \
        }                                                                      \
        for (size_t __i = __target_pos; __i < __stop_pos-1; ++__i)             \
        {                                                                      \
            memmove(&((p_map->_table)[__i]),                                   \
                &((p_map->_table)[(__i + 1) % __table_len]),                   \
                sizeof(p_map->_tmp));                                          \
        }                                                                      \
        (p_map->_table)[__stop_pos-1]._in_use = false;                         \
        --(p_map->_nelem);                                                     \
        p_map->status = MAP_SUCCESS;                                           \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        p_map->status = MAP_KEY_NOT_FOUND;                                     \
    }                                                                          \
}while(0)

#define _map_length(p_ans, p_map)                                              \
do                                                                             \
{                                                                              \
    *p_ans = p_map->_nelem;                                                    \
}while(0)

#define _map_load_factor(p_ans, p_map)                                         \
do                                                                             \
{                                                                              \
    *p_ans = ((double)p_map->_nelem)/_map_pow2(p_map->_bits);                  \
}while(0)

static size_t str_hash(void* key)
{
    char* str = (char*)key;
    // Hash cstring using the jdb hash function.
    size_t hash = 5381;
    while (*str)
    {
        hash = 33 * hash ^ (unsigned char)*str++;
    }
    return hash;
}

static bool str_eq(void* str1, void* str2)
{
    return strcmp(str1, str2) == 0;
}

#endif // !_MAP_H_
