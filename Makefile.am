AUTOMAKE_OPTIONS = foreign
SUBDIRS = src tests

test: 
	make -C src cvm
	src/cvm test.cvm
