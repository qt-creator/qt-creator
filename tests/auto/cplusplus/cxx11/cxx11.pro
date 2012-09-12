include(../../qttest.pri)
include(../shared/shared.pri)

DEFINES+=TESTSRCDIR=\\\"$$PWD\\\"

SOURCES += tst_cxx11.cpp
OTHER_FILES += \
    data/inlineNamespace.1.cpp \
    data/inlineNamespace.1.errors.txt \
    data/staticAssert.1.cpp \
    data/staticAssert.1.errors.txt \
    data/noExcept.1.cpp \
    data/noExcept.1.errors.txt
