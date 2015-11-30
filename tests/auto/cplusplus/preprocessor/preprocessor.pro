include(../shared/shared.pri)

# Inject the source dir for referencing test data from shadow builds.
DEFINES += SRCDIR=\\\"$$PWD\\\"

SOURCES += tst_preprocessor.cpp
DISTFILES = \
    data/noPP.1.cpp \
    data/noPP.2.cpp \
    data/identifier-expansion.1.cpp data/identifier-expansion.1.out.cpp \
    data/identifier-expansion.2.cpp data/identifier-expansion.2.out.cpp \
    data/identifier-expansion.3.cpp data/identifier-expansion.3.out.cpp \
    data/identifier-expansion.4.cpp data/identifier-expansion.4.out.cpp \
    data/identifier-expansion.5.cpp data/identifier-expansion.5.out.cpp \
    data/reserved.1.cpp data/reserved.1.out.cpp \
    data/recursive.1.cpp data/recursive.1.out.cpp \
    data/macro_expand.c data/macro_expand.out.c \
    data/macro_expand_1.cpp data/macro_expand_1.out.cpp \
    data/macro-test.cpp data/macro-test.out.cpp \
    data/poundpound.1.cpp data/poundpound.1.out.cpp \
    data/empty-macro.cpp data/empty-macro.out.cpp \
    data/empty-macro.2.cpp data/empty-macro.2.out.cpp \
    data/macro_pounder_fn.c
