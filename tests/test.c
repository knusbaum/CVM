#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "map_test.h"

Suite * cvm_suite(void)
{
    Suite *s;
    s = suite_create("CVM");

    TCase *tc_maps = map_testcases();

    suite_add_tcase(s, tc_maps);
    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = cvm_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}