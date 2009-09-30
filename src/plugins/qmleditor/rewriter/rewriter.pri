
INCLUDEPATH += $$PWD
HEADERS += $$PWD/textwriter_p.h
SOURCES += $$PWD/textwriter.cpp

!no_ast_rewriter {
    HEADERS += $$PWD/rewriter_p.h
    SOURCES += $$PWD/rewriter.cpp
}
