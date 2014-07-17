QTC_LIB_DEPENDS += utils
include(../../qttest.pri)

*-g++ {
    CONFIG -= warn_on
    QMAKE_CXXFLAGS += -Wall -Wno-trigraphs
}

SOURCES += tst_fileutils.cpp
