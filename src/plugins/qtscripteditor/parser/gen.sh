#!/bin/sh

me=$(dirname $0)

rm -f javascript.g
rm -f javascriptast.cpp
rm -f javascriptast_p.h
rm -f javascriptastfwd_p.h
rm -f javascriptastvisitor.cpp
rm -f javascriptastvisitor_p.h
rm -f javascriptlexer.cpp
rm -f javascriptlexer_p.h
rm -f javascriptmemorypool_p.h
rm -f javascriptnodepool_p.h

rm -f javascriptgrammar_p.h
rm -f javascriptgrammar.cpp
rm -f javascriptparser_p.h
rm -f javascriptparser.cpp

sed -f $me/cmd.sed $QTDIR/src/script/qscript.g > javascript.g

sed -f $me/cmd.sed $QTDIR/src/script/qscriptast.cpp > javascriptast.cpp
sed -f $me/cmd.sed $QTDIR/src/script/qscriptast_p.h > javascriptast_p.h
sed -f $me/cmd.sed $QTDIR/src/script/qscriptastfwd_p.h > javascriptastfwd_p.h
sed -f $me/cmd.sed $QTDIR/src/script/qscriptastvisitor.cpp > javascriptastvisitor.cpp
sed -f $me/cmd.sed $QTDIR/src/script/qscriptastvisitor_p.h > javascriptastvisitor_p.h
sed -f $me/cmd.sed $QTDIR/src/script/qscriptlexer_p.h > javascriptlexer_p.h
sed -f $me/cmd.sed $QTDIR/src/script/qscriptlexer.cpp > javascriptlexer.cpp
sed -f $me/cmd.sed $QTDIR/src/script/qscriptmemorypool_p.h > javascriptmemorypool_p.h
sed -f $me/cmd.sed $QTDIR/src/script/qscriptnodepool_p.h > javascriptnodepool_p.h

qlalr --troll --no-lines --no-debug $me/javascript.g

chmod ugo-w javascript.g
chmod ugo-w javascriptast.cpp
chmod ugo-w javascriptast_p.h
chmod ugo-w javascriptastfwd_p.h
chmod ugo-w javascriptastvisitor.cpp
chmod ugo-w javascriptastvisitor_p.h
chmod ugo-w javascriptlexer_p.h
chmod ugo-w javascriptlexer.cpp
chmod ugo-w javascriptmemorypool_p.h
chmod ugo-w javascriptnodepool_p.h

chmod ugo-w javascriptgrammar_p.h
chmod ugo-w javascriptgrammar.cpp
chmod ugo-w javascriptparser_p.h
chmod ugo-w javascriptparser.cpp
