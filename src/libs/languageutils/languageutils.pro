TEMPLATE = lib
TARGET = LanguageUtils
DEFINES += QT_CREATOR QT_NO_CAST_FROM_ASCII

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(languageutils-lib.pri)
include(../utils/utils.pri)
