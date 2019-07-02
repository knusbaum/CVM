#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <check.h>
#include "../src/map.h"
#include "../src/parser.h"
#include "../src/vm.h"
#include "../config.h"
#include "harness.h"

extern uintptr_t stack[STACKSIZE];
extern uintptr_t registers[15];
extern uintptr_t flags;

START_TEST(test_movrr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R1 $10\n"
        "mov R0 R1\n"
        "exit\n"
        );

    ck_assert_msg(registers[R0] == 10,
                  "Expected register R0 == 10, but R0 == %d", registers[R0]);
}
END_TEST

START_TEST(test_movrc)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R0 $10\n"
        "exit\n"
        );

    ck_assert_msg(registers[R0] == 10,
                  "Expected register R0 == 10, but R0 == %d", registers[R0]);
}
END_TEST

START_TEST(test_struct_new_movro_movor_movoc)
{
    PRINT_TEST_NAME
    run_testcode(
        // Create struct foo (struct)
        "struct foo\n"
        "bar\n"
        "endstruct\n"

        "start:\n"
        // Create a foo (new)
        "new R0 foo\n"

        // Populate foo.bar with 10 (movor)
        "mov R1 0xFEEDBAAC\n"
        "mov R0(foo.bar) R1\n"

        // move foo.bar into R2 (movro)
        "mov R2 R0(foo.bar)\n"

        // Populate foo.bar with $12 (movoc)
        "mov R0(foo.bar) $12\n"
        "mov R3 R0(foo.bar)\n"
        "exit\n"
        );

    ck_assert_msg(registers[R0] != 0,
                  "Expected register R0 != 0, but R0 == %d", registers[R0]);
    ck_assert_msg(registers[R2] == 0xFEEDBAAC,
                  "Expected register R2 == 0xFEEDBAAC, but R2 == %.16lX", registers[R2]);
    ck_assert_msg(registers[R3] == 12,
                  "Expected register R3 == 12, but R3 == %d", registers[R3]);

}
END_TEST

START_TEST(test_array_indexing)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "new R0 $100\n"
        "mov R0($8)[$0] 0xDEADBEEFCAFEBABE\n"

        "mov R1 R0($1)[$2]\n"

        "exit\n"
        );

    ck_assert_msg(registers[R1] == 0xFE,
                  "Expected register R1 == 0xFE, but R1 == %.16lX", registers[R1]);

}
END_TEST

START_TEST(test_array_new_movro_movor_movoc)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        // Create array of 100 bytes (new)
        "new R0 $100\n"

        // Populate the third word of the array with 0xFEFEFEFEFEFEFEFE (movoc)
        "mov R0($8)[$2] 0xFEFEFEFEFEFEFEFE\n"

        // Move the third word of the array into R1 (movro)
        "mov R1 R0($8)[$2]\n"

        // Populate the fifth word of the array with 0xADADADADADADADAD (movor)
        "mov R4 0xADADADADADADADAD\n"
        "mov R0($8)[$4] R4\n"
        "mov R3 R0($8)[$4]\n"


        // Populate the first word of the array with 0xDEADBEEFCAFEBABE (movor)
        "mov R4 0xDEADBEEFCAFEBABE\n"
        "mov R0($8)[$0] R4\n"
        // Put the third byte into R5 (movro)
        "mov R5 R0($1)[$2]\n"
        "mov R6 R0($8)[$0]\n"

        "exit\n"
        );

    ck_assert_msg(registers[R0] != 0,
                  "Expected register R0 != 0, but R0 == %d", registers[R0]);
    ck_assert_msg(registers[R1] == 0xFEFEFEFEFEFEFEFE,
                  "Expected register R1 == 0xFEFEFEFEFEFEFEFE, but R1 == %.16lX", registers[R1]);
    ck_assert_msg(registers[R3] == 0xADADADADADADADAD,
                  "Expected register R3 == 0xADADADADADADADAD, but R3 == %.16lX", registers[R3]);
    ck_assert_msg(registers[R6] == 0xDEADBEEFCAFEBABE,
                  "Expected register R6 == 0xDEADBEEFCAFEBABE, but R6 == %.16lX", registers[R6]);
    ck_assert_msg(registers[R5] == 0xFE,
                  "Expected register R5 == 0xFE, but R5 == %.16lX", registers[R5]);

}
END_TEST

START_TEST(test_incr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "inc R0\n"
        "inc R1\n"
        "inc R1\n"
        "inc R1\n"
        "exit\n"
        );
    ck_assert_msg(registers[R0] == 1,
                  "Expected register R0 == 1, but R0 == %ld", registers[R0]);
    ck_assert_msg(registers[R1] == 3,
                  "Expected register R1 == 3, but R1 == %ld", registers[R1]);

}
END_TEST

START_TEST(test_cmprr_equal)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "cmp R0 R1\n"
        "exit\n"
        );
    ck_assert_msg(flags & FLAG_EQUAL,
                  "Expected flags & FLAG_EQUAL");
    ck_assert_msg(!(flags & FLAG_LESS),
                  "Expected !(flags & FLAG_LESS)");
    ck_assert_msg(!(flags & FLAG_GREATER),
                  "Expected !(flags & FLAG_GREATER)");
}
END_TEST

START_TEST(test_cmprr_less)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "inc R1\n"
        "cmp R0 R1\n"
        "exit\n"
        );
    ck_assert_msg(!(flags & FLAG_EQUAL),
                  "Expected !(flags & FLAG_EQUAL)");
    ck_assert_msg(flags & FLAG_LESS,
                  "Expected flags & FLAG_LESS");
    ck_assert_msg(!(flags & FLAG_GREATER),
                  "Expected !(flags & FLAG_GREATER)");
}
END_TEST

START_TEST(test_cmprr_greater)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "inc R0\n"
        "cmp R0 R1\n"
        "exit\n"
        );
    ck_assert_msg(!(flags & FLAG_EQUAL),
                  "Expected !(flags & FLAG_EQUAL)");
    ck_assert_msg(!(flags & FLAG_LESS),
                  "Expected !(flags & FLAG_LESS)");
    ck_assert_msg(flags & FLAG_GREATER,
                  "Expected flags & FLAG_GREATER");
}
END_TEST

START_TEST(test_cmprc_equal)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "cmp R0 $0\n"
        "exit\n"
        );
    ck_assert_msg(flags & FLAG_EQUAL,
                  "Expected flags & FLAG_EQUAL");
    ck_assert_msg(!(flags & FLAG_LESS),
                  "Expected !(flags & FLAG_LESS)");
    ck_assert_msg(!(flags & FLAG_GREATER),
                  "Expected !(flags & FLAG_GREATER)");
}
END_TEST

START_TEST(test_cmprc_less)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "cmp R0 $1\n"
        "exit\n"
        );
    ck_assert_msg(!(flags & FLAG_EQUAL),
                  "Expected !(flags & FLAG_EQUAL)");
    ck_assert_msg(flags & FLAG_LESS,
                  "Expected flags & FLAG_LESS");
    ck_assert_msg(!(flags & FLAG_GREATER),
                  "Expected !(flags & FLAG_GREATER)");
}
END_TEST

START_TEST(test_cmprc_greater)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "inc R0\n"
        "cmp R0 $0\n"
        "exit\n"
        );
    ck_assert_msg(!(flags & FLAG_EQUAL),
                  "Expected !(flags & FLAG_EQUAL)");
    ck_assert_msg(!(flags & FLAG_LESS),
                  "Expected !(flags & FLAG_LESS)");
    ck_assert_msg(flags & FLAG_GREATER,
                  "Expected flags & FLAG_GREATER");
}
END_TEST

START_TEST(test_addrr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R0 $10\n"
        "mov R1 $20\n"
        "add R0 R1\n"
        "exit\n"
        );
    ck_assert_msg(registers[R0] == 30,
                  "Expected register R0 == 30, but R0 == %ld", registers[R0]);
}
END_TEST

START_TEST(test_addrc)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R0 $10\n"
        "add R0 $20\n"
        "exit\n"
        );
    ck_assert_msg(registers[R0] == 30,
                  "Expected register R0 == 30, but R0 == %ld", registers[R0]);
}
END_TEST

START_TEST(test_subrr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R0 $20\n"
        "mov R1 $10\n"
        "sub R0 R1\n"
        "exit\n"
        );
    ck_assert_msg(registers[R0] == 10,
                  "Expected register R0 == 10, but R0 == %ld", registers[R0]);
}
END_TEST

START_TEST(test_subrc)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R0 $20\n"
        "sub R0 $10\n"
        "exit\n"
        );
    ck_assert_msg(registers[R0] == 10,
                  "Expected register R0 == 10, but R0 == %ld", registers[R0]);
}
END_TEST

START_TEST(test_jmpr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R0 IP\n"
        "inc R1\n"
        "cmp R1 $10\n"
        "jge end\n"
        "jmp R0\n"
        "end:\n"
        "exit\n"
        );
    ck_assert_msg(registers[R1] == 10,
                  "Expected register R1 == 10, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_jmp)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "jmp foo\n"
        "exit\n"

        "foo:\n"
        "mov R12 0x600D\n"
        "exit\n"
        );
    ck_assert_msg(registers[R12] == 0x600D,
                  "Expected register R12 == 0x600, but R12 == %ld", registers[R12]);
}
END_TEST

START_TEST(test_jer)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R0 IP\n"
        "inc R1\n"
        "cmp R1 $1\n"
        "je R0\n"
        "exit\n"
        );
    ck_assert_msg(registers[R1] == 2,
                  "Expected register R1 == 2, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_je)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R0 IP\n"
        "inc R1\n"
        "cmp R1 $1\n"
        "je foo\n"
        "exit\n"

        "foo:\n"
        "mov R11 0x600D\n"
        "exit\n"
        );
    ck_assert_msg(registers[R11] == 0x600D,
                  "Expected register R11 == 0x600D, but R11 == %.16lX", registers[R11]);
}
END_TEST

START_TEST(test_jner)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R0 IP\n"
        "inc R1\n"
        "cmp R1 $2\n"
        "jne R0\n"
        "exit\n"
        );
    ck_assert_msg(registers[R1] == 2,
                  "Expected register R1 == 2, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_jne)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R0 IP\n"
        "cmp R1 $1\n"
        "jne foo\n"
        "exit\n"

        "foo:\n"
        "mov R11 0x600D\n"
        "exit\n"
        );
    ck_assert_msg(registers[R11] == 0x600D,
                  "Expected register R11 == 0x600D, but R11 == %.16lX", registers[R11]);
}
END_TEST

START_TEST(test_jgr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R1 $10\n"
        "mov R0 IP\n"
        "sub R1 $1\n"
        "cmp R1 $1\n"
        "jg R0\n"
        "exit\n"
        );
    ck_assert_msg(registers[R1] == 1,
                  "Expected register R1 == 1, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_jg)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "loop:\n"
        "inc R1\n"
        "cmp R1 $10\n"
        "jg foo\n"
        "jmp loop\n"
        "exit\n"

        "foo:\n"
        "mov R11 0x600D\n"
        "exit\n"
        );
    ck_assert_msg(registers[R11] == 0x600D,
                  "Expected register R11 == 0x600D, but R11 == %.16lX", registers[R11]);
    ck_assert_msg(registers[R1] == 11,
                  "Expected register R1 == 11, but R1 == %ld", registers[R1]);

}
END_TEST

START_TEST(test_jger)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R1 $10\n"
        "mov R0 IP\n"
        "sub R1 $1\n"
        "cmp R1 $1\n"
        "jge R0\n"
        "exit\n"
        );
    ck_assert_msg(registers[R1] == 0,
                  "Expected register R1 == 0, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_jge)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "loop:\n"
        "mov R0 IP\n"
        "inc R1\n"
        "cmp R1 $10\n"
        "jge foo\n"
        "jmp loop\n"
        "exit\n"

        "foo:\n"
        "mov R11 0x600D\n"
        "exit\n"
        );
    ck_assert_msg(registers[R11] == 0x600D,
                  "Expected register R11 == 0x600D, but R11 == %.16lX", registers[R11]);
    ck_assert_msg(registers[R1] == 10,
                  "Expected register R1 == 10, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_jlr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R0 IP\n"
        "inc R1\n"
        "cmp R1 $10\n"
        "jl R0\n"
        "exit\n"
        );
    ck_assert_msg(registers[R1] == 10,
                  "Expected register R1 == 10, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_jl)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R1 $10\n"
        "loop:\n"
        "sub R1 $1\n"
        "cmp R1 $1\n"
        "jl foo\n"
        "jmp loop\n"
        "exit\n"

        "foo:\n"
        "mov R11 0x600D\n"
        "exit\n"
        );
    ck_assert_msg(registers[R11] == 0x600D,
                  "Expected register R11 == 0x600D, but R11 == %.16lX", registers[R11]);
    ck_assert_msg(registers[R1] == 0,
                  "Expected register R1 == 0, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_jler)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R0 IP\n"
        "inc R1\n"
        "cmp R1 $10\n"
        "jle R0\n"
        "exit\n"
        );
    ck_assert_msg(registers[R1] == 11,
                  "Expected register R1 == 11, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_jle)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"

        "mov R1 $10\n"
        "loop:\n"
        "sub R1 $1\n"
        "cmp R1 $1\n"
        "jle foo\n"
        "jmp loop\n"
        "exit\n"

        "foo:\n"
        "mov R11 0x600D\n"
        "exit\n"
        );
    ck_assert_msg(registers[R11] == 0x600D,
                  "Expected register R11 == 0x600D, but R11 == %.16lX", registers[R11]);
    ck_assert_msg(registers[R1] == 1,
                  "Expected register R1 == 1, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_newc)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R0 $0\n"
        "new R0 $10\n"
        "mov R0($1)[$0] $100\n"
        "mov R1 R0($1)[$0]\n"
        "exit\n"
        );
    ck_assert_msg(registers[R0] != 0,
                  "Expected register R0 != 0, but R0 == %ld", registers[R0]);
    ck_assert_msg(registers[R1] == 100,
                  "Expected register R1 == 100, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_newr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R1 $100\n"
        "new R0 R1\n"
        "mov R0($2)[$0] $1010\n"
        "mov R1 R0($2)[$0]\n"
        "exit\n"
        );
    ck_assert_msg(registers[R0] != 0,
                  "Expected register R0 != 0, but R0 == %ld", registers[R0]);
    ck_assert_msg(registers[R1] == 1010,
                  "Expected register R1 == 1010, but R1 == %ld", registers[R1]);
}
END_TEST

START_TEST(test_pushr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "mov R0 $100\n"
        "push R0\n"
        "exit\n"
        );
    ck_assert_msg(stack[0] != 0,
                  "Expected stack[0] == 100, but stack[0] == %ld", stack[0]);
}
END_TEST

START_TEST(test_pushc)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "push $100\n"
        "exit\n"
        );
    ck_assert_msg(stack[0] != 0,
                  "Expected stack[0] == 100, but stack[0] == %ld", stack[0]);
}
END_TEST

START_TEST(test_popr)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "push $100\n"
        "pop R8\n"
        "exit\n"
        );
    ck_assert_msg(registers[R8] == 100,
                  "Expected register R8 == 100, but register R8 == %ld", registers[R8]);
}
END_TEST

START_TEST(test_call)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "call testfunc\n"
        "exit\n"
        "testfunc:\n"
        "mov R10 $2020\n"
        "exit\n"
        );
    ck_assert_msg(registers[R10] == 2020,
                  "Expected register R10 == 2020, but register R10 == %ld", registers[R10]);
    ck_assert_msg(stack[0] != 0,
                  "Expected stack[0] == 100, but stack[0] == %ld", stack[0]);
}
END_TEST

START_TEST(test_ret)
{
    PRINT_TEST_NAME
    run_testcode(
        "start:\n"
        "call testfunc\n"
        "mov R0 $1010\n"
        "exit\n"
        "testfunc:\n"
        "mov R10 $2020\n"
        "ret\n"
        );
    ck_assert_msg(registers[R10] == 2020,
                  "Expected register R10 == 2020, but register R10 == %ld", registers[R10]);
    ck_assert_msg(registers[R0] == 1010,
                  "Expected register R0 == 1010, but register R0 == %ld", registers[R0]);
}
END_TEST

TCase *instruction_testcases() {
    TCase *tc = tcase_create("VM Instructions");

    tcase_add_test(tc, test_movrr);
    tcase_add_test(tc, test_movrc);
    tcase_add_test(tc, test_struct_new_movro_movor_movoc);
    tcase_add_test(tc, test_array_indexing);
    tcase_add_test(tc, test_array_new_movro_movor_movoc);
    tcase_add_test(tc, test_incr);

    tcase_add_test(tc, test_cmprr_equal);
    tcase_add_test(tc, test_cmprr_less);
    tcase_add_test(tc, test_cmprr_greater);

    tcase_add_test(tc, test_cmprc_equal);
    tcase_add_test(tc, test_cmprc_less);
    tcase_add_test(tc, test_cmprc_greater);

    tcase_add_test(tc, test_addrr);
    tcase_add_test(tc, test_addrc);

    tcase_add_test(tc, test_subrr);
    tcase_add_test(tc, test_subrc);

    tcase_add_test(tc, test_jmpr);
    tcase_add_test(tc, test_jmp);

    tcase_add_test(tc, test_jer);
    tcase_add_test(tc, test_je);

    tcase_add_test(tc, test_jner);
    tcase_add_test(tc, test_jne);

    tcase_add_test(tc, test_jgr);
    tcase_add_test(tc, test_jg);

    tcase_add_test(tc, test_jger);
    tcase_add_test(tc, test_jge);

    tcase_add_test(tc, test_jlr);
    tcase_add_test(tc, test_jl);

    tcase_add_test(tc, test_jler);
    tcase_add_test(tc, test_jle);

    tcase_add_test(tc, test_newc);
    tcase_add_test(tc, test_newr);

    tcase_add_test(tc, test_pushr);
    tcase_add_test(tc, test_pushc);
    tcase_add_test(tc, test_popr);

    tcase_add_test(tc, test_call);
    tcase_add_test(tc, test_ret);

    return tc;
}
