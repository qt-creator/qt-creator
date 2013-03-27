include(../qttest.pri)

INCLUDEPATH += $$IDE_SOURCE_TREE/src/shared
SOURCES += tst_ioutils.cpp \
    $$IDE_SOURCE_TREE/src/shared/proparser/ioutils.cpp
