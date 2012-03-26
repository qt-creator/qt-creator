include(../../qttest.pri)
include(../shared/shared.pri)
SOURCES += tst_preprocessor.cpp

OTHER_FILES = \
    data/noPP.1.cpp data/noPP.1.errors.txt \
    data/identifier-expansion.1.cpp data/identifier-expansion.1.out.cpp data/identifier-expansion.1.errors.txt \
    data/identifier-expansion.2.cpp data/identifier-expansion.2.out.cpp data/identifier-expansion.2.errors.txt \
    data/identifier-expansion.3.cpp data/identifier-expansion.3.out.cpp data/identifier-expansion.3.errors.txt \
    data/identifier-expansion.4.cpp data/identifier-expansion.4.out.cpp data/identifier-expansion.4.errors.txt \
    data/reserved.1.cpp data/reserved.1.out.cpp data/reserved.1.errors.txt \
    data/macro_expand.c data/macro_expand.out.c data/macro_expand.errors.txt \
    data/empty-macro.cpp data/empty-macro.out.cpp
