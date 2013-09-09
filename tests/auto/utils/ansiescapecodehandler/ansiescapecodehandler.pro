QTC_LIB_DEPENDS += utils
include(../../qttest.pri)

UTILSDIR = $$IDE_SOURCE_TREE/src/libs/

DEFINES += QTCREATOR_UTILS_LIB

SOURCES += \
    $$UTILSDIR/utils/ansiescapecodehandler.cpp \
    tst_ansiescapecodehandler.cpp
