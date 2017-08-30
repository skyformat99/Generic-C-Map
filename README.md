# Generic C Map

## Intro
This project was an attempt to create a general hash table data structure that will allow mappings to and from any C datatype. In short with this library you get a truely generic map in C at the cost of a somewhat ugly API and headaches that come with a macro-only implimentation. This library is a neat little experiment, but I would not reccomend it for any type of production code.

## Advantages/Disadvantages of the Library
+ Advantages
    + Keys/values can be any type, including structs
    + Open addressing with robin hood hashing is cache friendly for table operations
    + Straghtforward API (what you see is what you get)
    + Header only library requires no extra linking
+ Disadvantages
    + All map operations are implimented through macros meaning:
        + Certain macro parameters **MUST** be constant expressions and will generate no compile-time warnings if you forget
        + All map operations use a void-function-like way of "returning" values that is ugly and unintuative
        + There are probably bugs I have not found yet because the C gods have a rule that all code written using macros comes with a minumum of 2 uncaught bugs upon release
    + I'm far from a hash table expert, so this library is probably slower than actual professionally developed libraries
    + No dynamic resizing, i.e. the size of the map is set only once during initialization
    + As far as I know, the library only compiles without warnings (using -Wall and -Wexra) with gcc. Clang genrally has more verbose warning messages which it *loves* printing for macro-only libraries

## API
Note: Due to the limitations of a macro-only library the macro parameters **map** and **ans** **MUST** be constant expressions.

The status (success or various different types of failures) can be checked after every operation with the `.status` member in the map.

----
### map declaration
Declare an anonymous map struct from `KEY_TYPE` to `VALUE_TYPE`.
```C
map(KEY_TYPE, VALUE_TYPE)
```
Example:
```C
// delcares my_map as a map from char* to float
map(char*, float) my_map;
```

----
### map_init
Initialize members of a newly declared map. Every map must be initialized before use. The user will provide two functions `hash_f` and `key_eq_f` that hashes/compares keys. The library comes with hash/eq functions for 32 and 64 bit integers as well as cstrings.
```C
map_init(map, hash_f, key_eq_f, num_bits)
```
Parameters:
+ `map` : target map to initialize **(required constexpr)**
+ `size_t (*)(void*) hash_f` : hashes a pointer-to-keytype to a size_t
+ `bool (*)(void*, void*) key_eq_f` : returns true if two keys are semantically equal
+ `unsigned num_bits` : the number of slots in the hash table is 2^num_bits

----
### map_deinit
Free resources used by a map. Every map should be deinitialized before leaving scope.
```C
map_deinit(map)
```
Parameters:
+ `map` : target map to deinitialize

----
### map_set
Performes the associative array operation `map[key] = value;`, overwriting the previous value at `map[key]` if it was already in the hash table.
```C
map_set(map, key, value)
```
Parameters:
+ `map` : target map **(required constexpr)**
+ `KEY_TYPE key` : target key
+ `VALUE_TYPE value` : value to that `map[key]` is set to

----
### map_get
Retrieve the value for a given key. Equivalent to the associative array operation `ans = map[key];`.
```C
map_get(ans, map, key)
```
Parameters:
+ `VALUE_TYPE ans` : lvalue to store the retrieved value in **(required constexpr)**
+ `map` : target map **(required constexpr)**
+ `KEY_TYPE key` : target key

----
### map_key_exists
Sets `ans` to `true` if `map[key]` has been set.
```C
map_key_exists(ans, map, key)
```
Parameters:
+ `bool ans` : lvalue set to `true` if `map[key]` has been set **(required constexpr)**
+ `map` : target map **(required constexpr)**
+ `KEY_TYPE key` : target key

----
### map_remove
Remove the value of `map[key]` from the map.
```C
map_remove(map, key)
```
Parameters:
+ `map` : target map **(required constexpr)**
+ `KEY_TYPE key` : target key

----
### map_length
Sets `ans` to the number of unique keys inserted into `map`.
```C
map_length(ans, map)
```
Parameters:
+ `size_t ans` : lvalue set to the number of key in the `map` **(required constexpr)**
+ `map` : target map **(required constexpr)**

----
## map_load_factor
Sets `ans` to the load factor `map`.
```C
map_load_factor(ans, map)
```
Parameters:
+ `double ans` : lvalue set to the load factor of `map` **(required constexpr)**
+ `map` : target map **(required constexpr)**

## License
MIT
