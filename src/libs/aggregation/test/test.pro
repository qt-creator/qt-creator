CONFIG += qtestlib
TEMPLATE = app
CONFIG -= app_bundle
DEFINES += AGGREGATION_LIBRARY

INCLUDEPATH += ../
# Input
SOURCES += tst_aggregate.cpp \
    ../aggregate.cpp
HEADERS += ../aggregate.h \
    ../aggregation_global.h

