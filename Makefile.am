AUTOMAKE_OPTIONS = foreign
SUBDIRS = src tests

cvm:
	make -C src cvm

test: cvm
	src/cvm examples/test.cvm

test2: cvm
	src/cvm examples/test2.cvm

test3: cvm
	src/cvm examples/test3.cvm

test4: cvm
	src/cvm examples/test4.cvm

testlist: cvm
	src/cvm examples/testlist.cvm

fib: cvm
	src/cvm examples/fib.cvm

