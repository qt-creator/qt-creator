include(../shared/shared.pri)

# Inject the source dir for referencing test data from shadow builds.
DEFINES += SRCDIR=\\\"$$PWD\\\"

SOURCES += tst_cxx11.cpp
DISTFILES += \
    data/inlineNamespace.1.cpp \
    data/inlineNamespace.1.errors.txt \
    data/nestedNamespace.1.cpp \
    data/nestedNamespace.1.errors.txt \
    data/staticAssert.1.cpp \
    data/staticAssert.1.errors.txt \
    data/noExcept.1.cpp \
    data/noExcept.1.errors.txt
