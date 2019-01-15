#ifndef ERRORS_H
#define ERRORS_H

#include <stdarg.h>

void err(const char * format, ...);
void fatal(const char * format, int exitCode, ...);
void warn(const char * format, ...);
void info(const char * format, ...);
void print(const char *format, ...);

#endif
