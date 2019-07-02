AM_CFLAGS = -ggdb -std=gnu11 -Wall -Werror
AM_LDFLAGS = -lgc

pkginclude_HEADERS=errors.h lexer.h parser.h vm.h map.h buffwriter.h

lib_LTLIBRARIES=libcvm.la
libcvm_la_SOURCES = errors.c lexer.c parser.c vm.c map.c buffwriter.c

bin_PROGRAMS = cvm
cvm_SOURCES = main.c
cvm_LDADD = libcvm.la
## For reference: https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
libcvm_la_LDFLAGS = -version-info 0:0:0
