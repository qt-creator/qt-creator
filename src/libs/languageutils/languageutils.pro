TEMPLATE = lib
TARGET = LanguageUtils
DEFINES += QT_CREATOR

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(languageutils-lib.pri)
include(../utils/utils.pri)
