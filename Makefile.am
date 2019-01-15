AUTOMAKE_OPTIONS = foreign
SUBDIRS = src tests

cvm:
	make -C src cvm

test: cvm
	src/cvm test.cvm

test2: cvm
	src/cvm test2.cvm

test3: cvm
	src/cvm test3.cvm

test4: cvm
	src/cvm test4.cvm

fib: cvm
	src/cvm fib.cvm

