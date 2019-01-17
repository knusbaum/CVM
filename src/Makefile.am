AM_CFLAGS = -ggdb -std=gnu11 -Wall -Werror
AM_LDFLAGS = -lgc

bin_PROGRAMS = cvm
cvm_SOURCES = main.c errors.c lexer.c parser.c vm.c map.c buffwriter.c

noinst_LIBRARIES=libcvm.a
libcvm_a_SOURCES=errors.c lexer.c parser.c vm.c map.c buffwriter.c
