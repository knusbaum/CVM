AM_CFLAGS = -ggdb -O0 -std=c11 -pedantic -Wall -Werror
AM_LDFLAGS = -lgc

#bin_PROGRAMS=test
#test_SOURCES=test.c

TESTS = check_cvm
check_PROGRAMS=check_cvm
check_cvm_SOURCES=test.c map_test.c test_instructions.c
check_cvm_CFLAGS=@CHECK_CFLAGS@
check_cvm_LDADD = $(top_builddir)/src/libcvm.a @CHECK_LIBS@
