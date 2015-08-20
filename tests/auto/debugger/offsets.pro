QT -= gui widgets
include(../qttest.pri)

QT += core-private

CONFIG -= c++11 # Fails to build with boost (due to #define private public)

exists(/usr/include/boost/unordered/unordered_set.hpp) {
    DEFINES += HAS_BOOST
}

exists(/usr/local/include/boost/unordered/unordered_set.hpp) {
    DEFINES += HAS_BOOST
    INCLUDEPATH += /usr/local/include
}

SOURCES += tst_offsets.cpp
