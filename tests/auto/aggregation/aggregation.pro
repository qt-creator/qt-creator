CONFIG += qtestlib
TEMPLATE = app
CONFIG -= app_bundle
DEFINES += AGGREGATION_LIBRARY

AGGREGATION_PATH = ../../../src/libs/aggregation

INCLUDEPATH += $$AGGREGATION_PATH
# Input
SOURCES += tst_aggregate.cpp \
    $$AGGREGATION_PATH/aggregate.cpp
HEADERS += $$AGGREGATION_PATH/aggregate.h \
    $$AGGREGATION_PATH/aggregation_global.h

TARGET=tst_$$TARGET
