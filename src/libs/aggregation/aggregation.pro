TEMPLATE = lib
TARGET = Aggregation

include(../../qtcreatorlibrary.pri)

DEFINES += AGGREGATION_LIBRARY QT_NO_CAST_FROM_ASCII

HEADERS = aggregate.h \
    aggregation_global.h

SOURCES = aggregate.cpp

