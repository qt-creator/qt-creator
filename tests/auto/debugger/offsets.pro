QT -= gui widgets
include(../qttest.pri)
include(../../../src/private_headers.pri)

QT += core-private

exists(/usr/include/boost/unordered/unordered_set.hpp) {
    DEFINES += HAS_BOOST
}

exists(/usr/local/include/boost/unordered/unordered_set.hpp) {
    DEFINES += HAS_BOOST
    INCLUDEPATH += /usr/local/include
}

SOURCES += tst_offsets.cpp
