#!/bin/sh

me=$(dirname $0)
cat $me/specs/glsl.g.in $me/specs/grammar.txt > $me/glsl.g
qlalr --no-lines --no-debug $me/glsl.g

