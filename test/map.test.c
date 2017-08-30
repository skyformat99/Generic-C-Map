#if __linux__
#   define _EMU_ENABLE_COLOR_
#endif
#include <EMUtest.h>
#include "../map.h"

size_t test_hash_int(void* key)
{
    // hash a key to itself for testing purposes
    return *(int*)key;
}

bool int_eq(void* inta, void* intb)
{
    return *(int*)inta == *(int*)intb;
}

EMU_TEST(init_and_deinit)
{
    map(int, int) m;

    map_init(m, test_hash_int, int_eq, 100);
    EMU_REQUIRE_EQ(m.status, MAP_INPUT_OUT_OF_RANGE);

    map_init(m, test_hash_int, int_eq, 16);
    EMU_REQUIRE_EQ(m.status, MAP_SUCCESS);
    map_deinit(m);

    map_init(m, test_hash_int, int_eq, MAP_DEFAULT_BITS);
    EMU_REQUIRE_EQ(m.status, MAP_SUCCESS);
    map_deinit(m);

    EMU_END_TEST();
}

EMU_TEST(basic_set)
{
    map(int, int) m;
    map_init(m, test_hash_int, int_eq, MAP_DEFAULT_BITS);

    map_set(m, 0, 3);
    EMU_REQUIRE_EQ(m.status, MAP_SUCCESS);
    EMU_EXPECT_EQ_UINT(m._nelem, 1);
    EMU_EXPECT_EQ_INT((m._table)[0]._key, 0);
    EMU_EXPECT_EQ_INT((m._table)[0]._value, 3);

    map_set(m, 1, 5);
    EMU_REQUIRE_EQ(m.status, MAP_SUCCESS);
    EMU_EXPECT_EQ_UINT(m._nelem, 2);
    EMU_EXPECT_EQ_INT((m._table)[1]._key, 1);
    EMU_EXPECT_EQ_INT((m._table)[1]._value, 5);

    map_set(m, 1, 7);
    EMU_REQUIRE_EQ(m.status, MAP_SUCCESS);
    EMU_EXPECT_EQ_UINT(m._nelem, 2);
    EMU_EXPECT_EQ_INT((m._table)[1]._key, 1);
    EMU_EXPECT_EQ_INT((m._table)[1]._value, 7);

    map_deinit(m);
    EMU_END_TEST();
}

EMU_TEST(complex_set_with_swaps)
{
    // Test insertion using the insertion example found at
    // http://codecapsule.com/2013/11/11/robin-hood-hashing/
    map(int, char) m;
    map_init(m, test_hash_int, int_eq, MAP_DEFAULT_BITS);
    size_t table_len = _map_pow2(m._bits);

    // setup before final insert
    map_set(m, 0, 'a');
    map_set(m, 0 + 1*table_len, 'b');
    map_set(m, 2, 'c');
    map_set(m, 2 + 1*table_len, 'd');
    map_set(m, 2 + 2*table_len, 'r');
    map_set(m, 4, 's');
    map_set(m, 4 + 1*table_len, 't');
    EMU_EXPECT_EQ_CHAR((m._table)[0]._value, 'a');
    EMU_EXPECT_EQ_CHAR((m._table)[1]._value, 'b');
    EMU_EXPECT_EQ_CHAR((m._table)[2]._value, 'c');
    EMU_EXPECT_EQ_CHAR((m._table)[3]._value, 'd');
    EMU_EXPECT_EQ_CHAR((m._table)[4]._value, 'r');
    EMU_EXPECT_EQ_CHAR((m._table)[5]._value, 's');
    EMU_EXPECT_EQ_CHAR((m._table)[6]._value, 't');

    // final insert
    map_set(m, 3, 'x');
    EMU_EXPECT_EQ_CHAR((m._table)[0]._value, 'a');
    EMU_EXPECT_EQ_CHAR((m._table)[1]._value, 'b');
    EMU_EXPECT_EQ_CHAR((m._table)[2]._value, 'c');
    EMU_EXPECT_EQ_CHAR((m._table)[3]._value, 'd');
    EMU_EXPECT_EQ_CHAR((m._table)[4]._value, 'r');
    EMU_EXPECT_EQ_CHAR((m._table)[5]._value, 'x');
    EMU_EXPECT_EQ_CHAR((m._table)[6]._value, 't');
    EMU_EXPECT_EQ_CHAR((m._table)[7]._value, 's');

    map_deinit(m);
    EMU_END_TEST();
}

EMU_TEST(probe_over_table_boundry)
{
    map(int, int) m;
    map_init(m, test_hash_int, int_eq, 2);

    map_set(m, 3, 98);
    map_set(m, 7, 99);
    EMU_EXPECT_EQ(m.status, MAP_SUCCESS);
    EMU_EXPECT_EQ_INT((m._table)[3]._value, 98);
    EMU_EXPECT_EQ_INT((m._table)[0]._value, 99);

    map_deinit(m);
    EMU_END_TEST();
}

EMU_GROUP(map_set)
{
    EMU_ADD(basic_set);
    EMU_ADD(complex_set_with_swaps);
    EMU_ADD(probe_over_table_boundry);
    EMU_END_GROUP();
}

EMU_TEST(map_key_exists)
{
    map(int, int) m;
    map_init(m, test_hash_int, int_eq, MAP_DEFAULT_BITS);
    bool ans;

    map_key_exists(ans, m, 0);
    EMU_REQUIRE_EQ(m.status, MAP_SUCCESS);
    EMU_EXPECT_FALSE(ans);

    map_set(m, 0, 3);
    map_key_exists(ans, m, 0);
    EMU_REQUIRE_EQ(m.status, MAP_SUCCESS);
    EMU_EXPECT_TRUE(ans);

    map_deinit(m);
    EMU_END_TEST();
}

EMU_TEST(map_get)
{
    map(int, int) m;
    map_init(m, test_hash_int, int_eq, MAP_DEFAULT_BITS);

    int ans;
    map_get(ans, m, 0);
    EMU_EXPECT_EQ(m.status, MAP_KEY_NOT_FOUND);

    map_set(m, 0, 3);
    map_get(ans, m, 0);
    EMU_REQUIRE_EQ(m.status, MAP_SUCCESS);
    EMU_EXPECT_EQ_INT(ans, 3);

    map_deinit(m);
    EMU_END_TEST();
}

EMU_TEST(basic_remove)
{
    map(int, int) m;
    map_init(m, test_hash_int, int_eq, MAP_DEFAULT_BITS);
    bool ans;

    map_set(m, 0, 3);
    map_remove(m, 0);
    EMU_REQUIRE_EQ(m.status, MAP_SUCCESS);
    EMU_EXPECT_EQ_UINT(m._nelem, 0);
    map_key_exists(ans, m, 0);
    EMU_EXPECT_FALSE(ans);

    map_remove(m, 0);
    EMU_REQUIRE_EQ(m.status, MAP_KEY_NOT_FOUND);

    map_deinit(m);
    EMU_END_TEST();
}

EMU_TEST(complex_remove_with_swaps)
{
    // Test remove using the remove with backwards shift example found at
    // http://codecapsule.com/2013/11/17/robin-hood-hashing-backward-shift-deletion
    // although the actual backwards shift deletion diagram on the page is
    // incorrect.
    map(int, char) m;
    map_init(m, test_hash_int, int_eq, MAP_DEFAULT_BITS);
    size_t table_len = _map_pow2(m._bits);

    // setup
    map_set(m, 0, 'a');
    map_set(m, 0 + 1*table_len, 'b');
    map_set(m, 2, 'c');
    map_set(m, 2 + 1*table_len, 'd');
    map_set(m, 2 + 2*table_len, 'r');
    map_set(m, 4, 's');
    map_set(m, 4 + 1*table_len, 't');
    EMU_EXPECT_EQ_CHAR((m._table)[0]._value, 'a');
    EMU_EXPECT_EQ_CHAR((m._table)[1]._value, 'b');
    EMU_EXPECT_EQ_CHAR((m._table)[2]._value, 'c');
    EMU_EXPECT_EQ_CHAR((m._table)[3]._value, 'd');
    EMU_EXPECT_EQ_CHAR((m._table)[4]._value, 'r');
    EMU_EXPECT_EQ_CHAR((m._table)[5]._value, 's');
    EMU_EXPECT_EQ_CHAR((m._table)[6]._value, 't');

    map_remove(m, 2 + 1*table_len); // remove 'd'
    EMU_EXPECT_EQ_CHAR((m._table)[0]._value, 'a');
    EMU_EXPECT_EQ_CHAR((m._table)[1]._value, 'b');
    EMU_EXPECT_EQ_CHAR((m._table)[2]._value, 'c');
    EMU_EXPECT_EQ_CHAR((m._table)[3]._value, 'r');
    EMU_EXPECT_EQ_CHAR((m._table)[4]._value, 's');
    EMU_EXPECT_EQ_CHAR((m._table)[5]._value, 't');

    map_deinit(m);
    EMU_END_TEST();
}

EMU_GROUP(map_remove)
{
    EMU_ADD(basic_remove);
    EMU_ADD(complex_remove_with_swaps);
    EMU_END_GROUP();
}

EMU_TEST(map_length)
{
    map(int, int) m;
    map_init(m, test_hash_int, int_eq, MAP_DEFAULT_BITS);
    size_t len;

    map_length(len, m);
    EMU_EXPECT_EQ_UINT(len, 0);

    map_set(m, 0, 3);
    map_length(len, m);
    EMU_EXPECT_EQ_UINT(len, 1);

    map_set(m, 0, 5);
    map_length(len, m);
    EMU_EXPECT_EQ_UINT(len, 1);

    map_set(m, 1, 7);
    map_length(len, m);
    EMU_EXPECT_EQ_UINT(len, 2);

    map_deinit(m);
    EMU_END_TEST();
}

EMU_TEST(map_load_factor)
{
    map(int, int) m;
    map_init(m, test_hash_int, int_eq, 2);
    double lf;

    map_load_factor(lf, m);
    EMU_EXPECT_FEQ(lf, 0, EMU_DEFAULT_EPSILON);

    map_set(m, 0, 3);
    map_load_factor(lf, m);
    EMU_EXPECT_FEQ(lf, 0.25, EMU_DEFAULT_EPSILON);

    map_set(m, 0, 5);
    map_load_factor(lf, m);
    EMU_EXPECT_FEQ(lf, 0.25, EMU_DEFAULT_EPSILON);

    map_set(m, 1, 7);
    map_load_factor(lf, m);
    EMU_EXPECT_FEQ(lf, 0.5, EMU_DEFAULT_EPSILON);

    map_set(m, 2, 9);
    map_load_factor(lf, m);
    EMU_EXPECT_FEQ(lf, 0.75, EMU_DEFAULT_EPSILON);

    map_set(m, 3, 11);
    map_load_factor(lf, m);
    EMU_EXPECT_FEQ(lf, 1, EMU_DEFAULT_EPSILON);

    map_deinit(m);
    EMU_END_TEST();
}

EMU_GROUP(macro_unit_tests)
{
    EMU_ADD(init_and_deinit);
    EMU_ADD(map_set);
    EMU_ADD(map_key_exists);
    EMU_ADD(map_get);
    EMU_ADD(map_remove);
    EMU_ADD(map_length);
    EMU_ADD(map_load_factor);
    EMU_END_GROUP();
}

EMU_TEST(book_reviews)
{
    // Example that follow the Rush HashMap example at
    // https://doc.rust-lang.org/std/collections/struct.HashMap.html
    map(char*, char*) book_reviews;
    map_init(book_reviews, str_hash, str_eq, MAP_DEFAULT_BITS);

    map_set(book_reviews, "Adventures of Huckleberry Finn", "My favorite book.");
    map_set(book_reviews, "Grimms' Fairy Tales", "Masterpiece.");
    map_set(book_reviews, "Pride and Prejudice", "Very enjoyable.");
    map_set(book_reviews, "The Adventures of Sherlock Holmes", "Eye lyked it alot.");

    char* review;
    map_get(review, book_reviews, "Adventures of Huckleberry Finn");
    EMU_EXPECT_STREQ(review, "My favorite book.");
    map_get(review, book_reviews, "Grimms' Fairy Tales");
    EMU_EXPECT_STREQ(review, "Masterpiece.");
    map_get(review, book_reviews, "Pride and Prejudice");
    EMU_EXPECT_STREQ(review, "Very enjoyable.");
    map_get(review, book_reviews, "The Adventures of Sherlock Holmes");
    EMU_EXPECT_STREQ(review, "Eye lyked it alot.");

    bool exists;
    map_key_exists(exists, book_reviews, "Les Misérables");
    if (!exists)
    {
        size_t num_elems;
        map_length(num_elems, book_reviews);
        EMU_PRINT_INDENT();
        printf("We've got %zu reviews, but Les Misérables ain't one.\n", num_elems);
    }

    map_remove(book_reviews, "The Adventures of Sherlock Holmes");

    // TODO: iterate over everything

    map_deinit(book_reviews);
    EMU_END_TEST();
}

void print_coffee_cost(void* map_param, char* coffee)
{
    map(char*, float) m;
    memcpy(&m, map_param, sizeof(map(char*, float))); // ugly but works

    float cost;
    map_get(cost, m, coffee);
    printf("%s : $%.2f\n", coffee, cost);
}

EMU_TEST(pass_map_to_a_function)
{
    map(char*, float) coffee_cost;
    map_init(coffee_cost, str_hash, str_eq, 4);

    map_set(coffee_cost, "black", 2.50);
    map_set(coffee_cost, "latte", 4.00);
    map_set(coffee_cost, "frap", 4.25);

    EMU_PRINT_INDENT(); print_coffee_cost(&coffee_cost, "black");
    EMU_PRINT_INDENT(); print_coffee_cost(&coffee_cost, "frap");
    EMU_PRINT_INDENT(); print_coffee_cost(&coffee_cost, "latte");

    map_deinit(coffee_cost);
    EMU_END_TEST();
}

EMU_GROUP(general_usability)
{
    EMU_ADD(book_reviews);
    EMU_ADD(pass_map_to_a_function);
    EMU_END_GROUP();
}

EMU_GROUP(all_tests)
{
    EMU_ADD(macro_unit_tests);
    EMU_ADD(general_usability);
    EMU_END_GROUP();
}

int main(void)
{
    return EMU_RUN(all_tests);
}
