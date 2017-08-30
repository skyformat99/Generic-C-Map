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

#define map_init(/* map(KEY_TYPE, VALUE_TYPE) */map,                           \
    /* size_t (*)(void*) */hash_f, /* int (*)(void*, void*) */key_eq_f,        \
    /* unsigned */num_bits)                                                    \
                                    _map_init((map), hash_f, key_eq_f, num_bits)

#define map_deinit(/* map(KEY_TYPE, VALUE_TYPE) */map)                         \
                                                              _map_deinit((map))

#define map_set(/* map(KEY_TYPE, VALUE_TYPE) */map, /* KEY_TYPE */key,         \
    /* VALUE_TYPE */value)                                                     \
                                                     _map_set((map), key, value)

#define map_get(/* VALUE_TYPE */ans, /* map(KEY_TYPE, VALUE_TYPE) */map,       \
    /* KEY_TYPE */key)                                                         \
                                                     _map_get((ans), (map), key)

#define map_key_exists(/* bool */ans, /* map(KEY_TYPE, VALUE_TYPE) */map,      \
    /* KEY_TYPE */key)                                                         \
                                              _map_key_exists((ans), (map), key)

#define map_remove(/* map(KEY_TYPE, VALUE_TYPE) */map, /* KEY_TYPE */key)      \
                                                         _map_remove((map), key)

#define map_length(/* size_t */ans, /* map(KEY_TYPE, VALUE_TYPE) */map)        \
                                                       _map_length((ans), (map))

#define map_load_factor(/* double */ans, /* map(KEY_TYPE, VALUE_TYPE) */map)   \
                                                  _map_load_factor((ans), (map))

// Hash/eq functions for built in types.
static size_t int32_hash(void* key);
static bool   int32_eq(void* i1, void* i2);
static size_t int64_hash(void* key);
static bool   int64_eq(void* i1, void* i2);
static size_t str_hash(void* key);
static bool   str_eq(void* str1, void* str2);

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

#define _map_init(map, hash_f, key_eq_f, num_bits)                             \
do                                                                             \
{                                                                              \
    map._bits = num_bits;                                                      \
    if (map._bits > MAP_BITS_PER_SIZE_T)                                       \
    {                                                                          \
        map.status = MAP_INPUT_OUT_OF_RANGE;                                   \
        break;                                                                 \
    }                                                                          \
    map._nelem = 0;                                                            \
    map._hash_f = hash_f;                                                      \
    map._key_eq_f = key_eq_f;                                                  \
    map._table =                                                               \
        calloc(_map_pow2(map._bits), sizeof(map._tmp));                        \
    if (map._table)                                                            \
        map.status = MAP_SUCCESS;                                              \
    else                                                                       \
        map.status = MAP_ALLOC_FAILURE;                                        \
}while(0)

#define _map_deinit(map)                                                       \
do                                                                             \
{                                                                              \
    free(map._table);                                                          \
    map._table = NULL;                                                         \
    map.status = MAP_SUCCESS;                                                  \
}while(0);

#define _map_set(map, key, value)                                              \
do                                                                             \
{                                                                              \
    /* prepare element to be set */                                            \
    map._tmp._key = key;                                                       \
    map._tmp._value = value;                                                   \
    map._tmp._hash = (map._hash_f)(&(map._tmp._key));                          \
    map._tmp._in_use = true;                                                   \
    /* insert using robin hood insertion */                                    \
    size_t __table_len = _map_pow2(map._bits);                                 \
    size_t __curr = map._tmp._hash % __table_len;                              \
    size_t __key_not_found = true;                                             \
    while (__key_not_found && (map._table)[__curr]._in_use)                    \
    {                                                                          \
        if (map._key_eq_f(&((map._table)[__curr]._key),                        \
            &(map._tmp._key)))                                                 \
        {                                                                      \
            /* key exists in the table already, so change it's value */        \
            memcpy(&((map._table)[__curr]), &(map._tmp),                       \
                sizeof(map._tmp));                                             \
            map.status = MAP_SUCCESS;                                          \
            __key_not_found = false;                                           \
            break;                                                             \
        }                                                                      \
        if (_map_dib(map._tmp._hash, __curr, map._bits) >                      \
            _map_dib((map._table)[__curr]._hash, __curr, map._bits))           \
        {                                                                      \
            _map_memswap(&(map._tmp), &((map._table)[__curr]),                 \
                sizeof(map._tmp));                                             \
        }                                                                      \
        __curr = (__curr + 1) % __table_len;                                   \
    }                                                                          \
    if (__key_not_found)                                                       \
    {                                                                          \
        /* insert element into first empty bucket */                           \
        memcpy(&((map._table)[__curr]), &(map._tmp),                           \
            sizeof(map._tmp));                                                 \
        /* adjust map metadata */                                              \
        ++(map._nelem);                                                        \
        map.status = MAP_SUCCESS;                                              \
    }                                                                          \
}while(0)

#define _map_get(ans, map, key)                                                \
do                                                                             \
{                                                                              \
    bool __key_exists;                                                         \
    _map_key_exists(__key_exists, map, key);                                   \
    if (__key_exists)                                                          \
    {                                                                          \
        /* _map_key_exists stores the target value, so extract it */           \
        ans = map._tmp._value;                                                 \
        map.status = MAP_SUCCESS;                                              \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        map.status = MAP_KEY_NOT_FOUND;                                        \
    }                                                                          \
}while(0)

#define _map_key_exists(ans, map, key)                                         \
do                                                                             \
{                                                                              \
    map._tmp._key = key;                                                       \
    map._tmp._hash = (map._hash_f)(&(map._tmp._key));                          \
    size_t __table_len = _map_pow2(map._bits);                                 \
    size_t __curr = map._tmp._hash % __table_len;                              \
    ans = false;                                                               \
    while ((map._table)[__curr]._in_use &&                                     \
        _map_dib(map._tmp._hash, __curr, map._bits) >=                         \
        _map_dib((map._table)[__curr]._hash, __curr, map._bits))               \
    {                                                                          \
        if (map._key_eq_f(&((map._table)[__curr]._key),                        \
            &(map._tmp._key)))                                                 \
        {                                                                      \
            ans = true;                                                        \
            /* map_get needs the value at __curr */                            \
            map._tmp._value = (map._table)[__curr]._value;                     \
            /* map_remove needs the location of the element (i.e. __curr) */   \
            /* use _hash as tmp index holder, not an actual hash */            \
            map._tmp._hash = __curr;                                           \
            break;                                                             \
        }                                                                      \
        __curr = (__curr + 1) % __table_len;                                   \
    }                                                                          \
    map.status = MAP_SUCCESS;                                                  \
}while(0)

#define _map_remove(map, key)                                                  \
do                                                                             \
{                                                                              \
    bool __key_exists;                                                         \
    _map_key_exists(__key_exists, map, key);                                   \
    if (__key_exists)                                                          \
    {                                                                          \
        /* _map_key_exists stores the index where target element was found */  \
        /* Note: _tmp._hash isn't actually a hash. It's the index where the */ \
        /* target element actually ended up. */                                \
        size_t __table_len = _map_pow2(map._bits);                             \
        size_t __target_pos = map._tmp._hash;                                  \
        size_t __stop_pos = (__target_pos + 1) % __table_len;                  \
        /* find position of the stop bucket */                                 \
        while ((map._table)[__stop_pos]._in_use &&                             \
            _map_dib(__target_pos, __stop_pos, map._bits) > 0)                 \
        {                                                                      \
            __stop_pos = (__stop_pos + 1) % __table_len;                       \
        }                                                                      \
        /* shift all elements in front of the stop bucket */                   \
        size_t __i = __target_pos;                                             \
        while (__i != __stop_pos-1)                                            \
        {                                                                      \
            memmove(&((map._table)[__i]),                                      \
                &((map._table)[(__i + 1) % __table_len]),                      \
                sizeof(map._tmp));                                             \
            __i = (__i + 1) % __table_len;                                     \
        }                                                                      \
        /* mark the duplicate bucket at the back as no longer in use */        \
        (map._table)[__stop_pos-1]._in_use = false;                            \
        /* adjust map metadata */                                              \
        --(map._nelem);                                                        \
        map.status = MAP_SUCCESS;                                              \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        map.status = MAP_KEY_NOT_FOUND;                                        \
    }                                                                          \
}while(0)

#define _map_length(ans, map)                                                  \
do                                                                             \
{                                                                              \
    ans = map._nelem;                                                          \
}while(0)

#define _map_load_factor(ans, map)                                             \
do                                                                             \
{                                                                              \
    ans = ((double)map._nelem)/_map_pow2(map._bits);                           \
}while(0)

static inline size_t djb_str(void* key)
{
    char* str = (char*)key;
    size_t hash = 5381;
    while (*str)
    {
        hash = 33 * hash ^ (unsigned char)*str++;
    }
    return hash;
}

static inline size_t djb_general(void* key, size_t size)
{
    char* bytes = (char*)key;
    size_t hash = 5381;
    for (size_t i = 0; i < size; ++i)
    {
        hash = 33 * hash ^ (unsigned char)bytes[i];
    }
    return hash;
}

#ifdef __GNUC__
#   define UNUSED __attribute__ ((unused))
#else
#   define UNUSED
#endif

UNUSED static size_t int32_hash(void* key)
{
    return djb_general(key, sizeof(uint32_t));
}

UNUSED static bool int32_eq(void* i1, void* i2)
{
    return *((int32_t*)i1) == *((int32_t*)i2);
}

UNUSED static size_t int64_hash(void* key)
{
    return djb_general(key, sizeof(uint64_t));
}

UNUSED static bool int64_eq(void* i1, void* i2)
{
    return *((int64_t*)i1) == *((int64_t*)i2);
}

UNUSED static size_t str_hash(void* key)
{
    return djb_str(key);
}

UNUSED static bool str_eq(void* str1, void* str2)
{
    return strcmp(str1, str2) == 0;
}

#endif // !_MAP_H_
