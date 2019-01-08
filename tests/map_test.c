#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "../src/map.h"

START_TEST(test_map_put_get)
{
    char x;
    map_t *m = map_create();
    map_put(m, "hello", &x);
    ck_assert_msg(map_get(m, "hello") == &x,
                  "Get returned bad value after put.");
    map_destroy(m);
}
END_TEST

START_TEST(test_map_put_get_multi)
{
    char x;
    char y;
    map_t *m = map_create();
    map_put(m, "hello", &x);
    map_put(m, "goodbye", &y);
    ck_assert_msg(map_get(m, "hello") == &x,
                  "Get returned bad value after put.");
    ck_assert_msg(map_get(m, "goodbye") == &y,
                  "Get returned bad value after multiple puts.");
    map_destroy(m);
}
END_TEST

START_TEST(test_map_put_overwrite)
{
    char x;
    char y;
    map_t *m = map_create();
    map_put(m, "hello", &x);
    ck_assert_msg(map_get(m, "hello") == &x,
                  "Get returned bad value after put.");
    map_put(m, "hello", &y);
    ck_assert_msg(map_get(m, "hello") == &y,
                  "Get returned bad value after put.");
    map_destroy(m);
}
END_TEST

START_TEST(test_big_map_stuff)
{
    // This is just so we can put unique pointers into the map
    char x[4096];

    map_t *m = map_create();
    for(int i = 0; i < 4096; i++) {
        char *str = malloc(8);
        sprintf(str, "key%d", i);
        map_put(m, str, &x[i]);
    }

    for(int i = 0; i < 4096; i++) {
        char str[8];
        sprintf(str, "key%d", i);
        ck_assert_msg(map_get(m, str) == &x[i],
                      "Failed to get key in big map.");
    }
}
END_TEST

START_TEST(test_get_without_put)
{
    map_t *m = map_create();
    ck_assert_msg(map_get(m, "hello") == NULL,
                  "GET didn't return NULL for bad key.");
}
END_TEST

START_TEST(test_map_delete)
{
    char x;
    map_t *m = map_create();
    ck_assert_msg(map_delete(m, "hello") == NULL,
                  "DELETE didn't return NULL.");
    map_put(m, "hello", &x);
    ck_assert_msg(map_delete(m, "hello") == &x,
                  "DELETE didn't return the value it deleted.");
    ck_assert_msg(map_get(m, "hello") == NULL,
                  "GET didn't return NULL for deleted key.");
}
END_TEST

START_TEST(test_big_map_delete_get)
{
    // This is just so we can put unique pointers into the map
    char x[4096];

    map_t *m = map_create();
    for(int i = 0; i < 4096; i++) {
        char *str = malloc(8);
        sprintf(str, "key%d", i);
        map_put(m, str, &x[i]);
    }

    for(int i = 1; i < 4096; i+=2) {
        char str[8];
        sprintf(str, "key%d", i);
        ck_assert_msg(map_delete(m, str) == &x[i],
                      "DELETE didn't return the value it deleted.");
    }

    for(int i = 0; i < 4096; i++) {
        char str[8];
        sprintf(str, "key%d", i);
        if(i % 2 == 0) {
            ck_assert_msg(map_get(m, str) == &x[i],
                          "Failed to get key in big map.");
        }
        else {
            ck_assert_msg(map_get(m, str) == NULL,
                          "Failed to get key in big map.");
        }
    }
}
END_TEST

START_TEST(test_map_null_key)
{
    map_t *m = map_create();
    map_put(m, "hello", m);
    ck_assert_msg(map_get(m, NULL) == NULL,
                  "GET didn't return NULL for NULL key.");
}
END_TEST

START_TEST(test_map_key_present)
{
    map_t *m = map_create();
    map_put(m, "hello", m);
    ck_assert_msg(map_present(m, "hello"),
                  "GET didn't return NULL for NULL key.");
}
END_TEST

TCase *map_testcases() {
    TCase *tc = tcase_create("Maps");

    tcase_add_test(tc, test_map_put_get);
    tcase_add_test(tc, test_map_put_get_multi);
    tcase_add_test(tc, test_map_put_overwrite);
    tcase_add_test(tc, test_big_map_stuff);
    tcase_add_test(tc, test_get_without_put);
    tcase_add_test(tc, test_map_delete);
    tcase_add_test(tc, test_big_map_delete_get);
    tcase_add_test(tc, test_map_null_key);
    tcase_add_test(tc, test_map_key_present);
    return tc;
}
