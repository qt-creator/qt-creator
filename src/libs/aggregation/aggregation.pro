TEMPLATE = lib
TARGET = Aggregation

include(../../qtcreatorlibrary.pri)

DEFINES += AGGREGATION_LIBRARY

HEADERS = aggregate.h \
    aggregation_global.h

SOURCES = aggregate.cpp

