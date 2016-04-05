#!/usr/bin/env bash

aclocal
autoreconf
automake --add-missing
autoconf
./configure
make

