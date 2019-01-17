#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <check.h>
#include "../src/map.h"
#include "../src/parser.h"
#include "../src/vm.h"

extern uintptr_t stack[STACKSIZE];
extern uintptr_t registers[15];
extern uintptr_t flags;

static FILE *preluded_file() {
    char template[] = "/tmp/test.cvm.XXXXXX";
    int fd = mkstemp(template);
    if(fd == -1) {
        char error[2048];
        int no = errno;
        sprintf(error, "Failed to create template file: (%d) %s\n",
                no, strerror(errno));
        ck_abort_msg(error);
        return NULL;
    }
    FILE *f = fdopen(fd, "r+");
    if(f == NULL) {
        char error[2048];
        sprintf(error, "Failed to create FILE * from descriptor: (%d) %s\n",
                errno, strerror(errno));
        ck_abort_msg(error);
    }
    char *prelude =
        "module test\n\n"
        "export start\n\n";
    fwrite(prelude, 1, strlen(prelude), f);

    return f;
}

static void filename(FILE *f, char *buffer, size_t buffsize) {
    int fd = fileno(f);
    char linkname[1024];
    sprintf(linkname, "/proc/self/fd/%d", fd);
    readlink(linkname, buffer, buffsize);
}

static void write_contents(FILE *f, char *contents) {
    fwrite(contents, 1, strlen(contents), f);
    fflush(f);
}

static void run_testcode(char *code) {
    FILE *f = preluded_file();
    char fbuffer[1024] = {0};
    filename(f, fbuffer, 1024);
    write_contents(f, code);
    fclose(f);
    struct module *m = load_module(fbuffer);
    run_module(m);
    unlink(fbuffer);
}

START_TEST(test_movrr)
{
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
    run_testcode(
        "start:\n"
        "mov R0 $10\n"
        "exit\n"
        );
    
    ck_assert_msg(registers[R0] == 10,
                  "Expected register R0 == 10, but R0 == %d", registers[R0]);
}
END_TEST

START_TEST(test_movro_movor_movoc)
{
    run_testcode(
        // Create struct foo
        "struct foo\n"
        "bar\n"
        "endstruct\n"
        
        "start:\n"
        // Create a foo
        "new R0 foo\n"

        // Populate foo.bar with 10 (movor)
        "mov R1 $10\n"
        "mov R0(foo.bar) R1\n"

        // move foo.bar into R2 (movro)
        "mov R2 R0(foo.bar)\n"

        // Populate foo.bar with $12 (movoc)
        "mov R0(foo.bar) $12\n"
        "mov R3 R0(foo.bar)\n"
        "exit\n"
        );

    ck_assert_msg(registers[R2] == 10,
                  "Expected register R2 == 10, but R2 == %d", registers[R2]);
    ck_assert_msg(registers[R3] == 12,
                  "Expected register R3 == 12, but R3 == %d", registers[R3]);
    
}
END_TEST

TCase *instruction_testcases() {
    TCase *tc = tcase_create("Instructions");

    tcase_add_test(tc, test_movrr);
    tcase_add_test(tc, test_movrc);
    tcase_add_test(tc, test_movro_movor_movoc);
    return tc;
}
