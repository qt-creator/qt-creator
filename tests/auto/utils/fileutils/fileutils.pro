include(../../qttest.pri)
include(../shared/shared.pri)

include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

UTILSDIR = $$IDE_SOURCE_TREE/src/libs/

INCLUDEPATH += $$UTILSDIR
DEFINES += QTCREATOR_UTILS_LIB

SOURCES += tst_fileutils.cpp \
    $$UTILSDIR/utils/fileutils.cpp \

