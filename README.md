# CVM
[![Build Status](https://travis-ci.com/knusbaum/CVM.svg?branch=master)](https://travis-ci.com/knusbaum/CVM)

CVM is a simple virtual machine that reads and executes an assembly-like lanugage

## Building
CVM uses autotools, so you need automake and autoconf installed.
You also need the boehm garbage collector installed on the system. See [http://www.hboehm.info/gc/](http://www.hboehm.info/gc/)

To generate the necessary build files:
```
$ ./setup.sh
```
You should only need to do this once.

Once this is done, you can build the `cvm` binary with make:
```
$ make
```

## Testing

Tests are located in the `tests` directory.

Generate build files if you haven't
```
$ ./setup.sh
```

Then:
```
$ make check
```

## Examples

Examples of how to write programs that CVM will execute can be found in the `examples` directory.

You can run many of these examples by using the targets in the makefile:
```
$ make test
$ make test2
$ make fib
$ make testlist
```

They may also be run manually:
```
$ src/cvm examples/testlist.cvm
```


## Notes
The output of `cvm` is extremely verbose, and there is no IO currently except for the `dumpreg` instruction.
I plan to change this in the future, but this is in early stages and under heavy development.

