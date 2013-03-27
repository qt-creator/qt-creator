TEMPLATE = lib
TARGET = Aggregation

include(../../qtcreatorlibrary.pri)
include(aggregation_depedencies.pri)

DEFINES += AGGREGATION_LIBRARY

HEADERS = aggregate.h \
    aggregation_global.h

SOURCES = aggregate.cpp

