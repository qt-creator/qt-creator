include(../../auto/qttest.pri)
DEFINES += FAKEVIM_STANDALONE \
    QTCREATOR_UTILS_STATIC_LIB

FAKEVIMDIR = $$IDE_SOURCE_TREE/src/plugins/fakevim
UTILSDIR = $$IDE_SOURCE_TREE/src/libs/utils

SOURCES += main.cpp \
            $$FAKEVIMDIR/fakevimhandler.cpp \
            $$FAKEVIMDIR/fakevimactions.cpp \
            $$UTILSDIR/hostosinfo.cpp \
            $$UTILSDIR/qtcassert.cpp

HEADERS += $$FAKEVIMDIR/fakevimhandler.h \
            $$FAKEVIMDIR/fakevimactions.h \
            $$UTILSDIR/hostosinfo.h \
            $$UTILSDIR/qtcassert.h

INCLUDEPATH += $$FAKEVIMDIR
