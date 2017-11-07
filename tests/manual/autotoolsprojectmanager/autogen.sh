#!/bin/sh

libtoolize
aclocal
automake --add-missing
autoconf
./configure
