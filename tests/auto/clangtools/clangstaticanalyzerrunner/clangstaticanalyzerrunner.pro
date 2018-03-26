include(../clangstaticanalyzertest.pri)

TARGET = tst_clangstaticanalyzerrunnertest

SOURCES += \
    tst_clangstaticanalyzerrunner.cpp \
    $$PLUGINDIR/clangstaticanalyzerrunner.cpp
HEADERS += \
    $$PLUGINDIR/clangstaticanalyzerrunner.h
