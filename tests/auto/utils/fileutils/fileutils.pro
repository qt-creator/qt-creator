QTC_LIB_DEPENDS += utils
include(../../qttest.pri)

DEFINES += QTCREATOR_UTILS_LIB
*-g++ {
    CONFIG -= warn_on
    QMAKE_CXXFLAGS += -Wall -Wno-trigraphs
}

SOURCES += tst_fileutils.cpp
