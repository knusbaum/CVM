#include "errors.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 2048

static void do_print(const char * prec, const char * format, va_list args) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, prec, BUFFER_SIZE - 1);
    strncat(buffer, format, BUFFER_SIZE - strlen(prec) - 1);
    buffer[BUFFER_SIZE-1] = 0;
    vfprintf(stderr, buffer, args);
}


void err(const char * format, ...) {
    va_list args;
    va_start(args, format);
    do_print("ERROR: ", format, args);
    va_end(args);
}

void fatal(const char * format, int exitCode, ...) {
    va_list args;
    va_start(args, exitCode);
    do_print("FATAL: ", format, args);
    va_end(args);
    exit(exitCode);
}

void warn(const char * format, ...) {
    va_list args;
    va_start(args, format);
    do_print("WARN: ", format, args);
    va_end(args);
}

void info(const char * format, ...) {
    va_list args;
    va_start(args, format);
    do_print("INFO: ", format, args);
    va_end(args);
}

void print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    do_print("", format, args);
    va_end(args);
}
