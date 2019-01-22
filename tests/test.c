#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "map_test.h"
#include "test_instructions.h"
#include "test_advanced.h"

Suite * cvm_suite(void)
{
    Suite *s;
    s = suite_create("CVM");

    TCase *tc_maps = map_testcases();
    TCase *tc_instructions = instruction_testcases();
    TCase *tc_advanced = advanced_testcases();

    suite_add_tcase(s, tc_maps);
    suite_add_tcase(s, tc_instructions);
    suite_add_tcase(s, tc_advanced);
    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = cvm_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
