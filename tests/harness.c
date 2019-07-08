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

void run_testcode(char *code) {
    FILE *f = preluded_file();
    char fbuffer[1024] = {0};
    filename(f, fbuffer, 1024);
    write_contents(f, code);
    fclose(f);
    struct module *m = load_module(fbuffer);
    run_module(m);
    unlink(fbuffer);
}
