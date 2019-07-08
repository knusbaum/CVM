#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <check.h>
#include <ffi.h>
#include "../src/map.h"
#include "../src/parser.h"
#include "../src/vm.h"
#include "../config.h"
#include "harness.h"

extern uintptr_t stack[STACKSIZE];
extern uintptr_t registers[15];
extern uintptr_t flags;

START_TEST(test_fibonacci)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R0 $30\n"
        "call fib\n"
        "exit\n"

        "fib:\n"
        "cmp R0 $2\n"
        "jle return_one\n"

        "push R0\n"
        "add R0 $-1\n"
        "call fib\n"

        // Restore our original target number
        "pop R0\n"
        // Save the result of fib(N - 1)
        "push R1\n"

        "add R0 $-2\n"
        "call fib\n"
        // fib(N - 2) is now in R1

        // Restore fib(N - 1) into R2
        "pop R2\n"

        // add the results
        "add R1 R2\n"
        "ret\n"


        "return_one:\n"
        "mov R1 $1\n"
        "ret\n"
        );
    ck_assert_msg(registers[R1] == 832040,
                  "Expected register R1 == 832040, but register R1 == %ld", registers[R1]);
}
END_TEST

TCase *advanced_testcases() {
    TCase *tc = tcase_create("Advanced Functionality");

    tcase_add_test(tc, test_fibonacci);
    return tc;
}
