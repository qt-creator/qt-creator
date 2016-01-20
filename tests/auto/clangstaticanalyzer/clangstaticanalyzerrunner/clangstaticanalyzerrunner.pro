include(../clangstaticanalyzertest.pri)

TARGET = tst_clangstaticanalyzerrunnertest

DEFINES += SRCDIR=\\\"$$PWD/\\\"

SOURCES += \
    tst_clangstaticanalyzerrunner.cpp \
    $$PLUGINDIR/clangstaticanalyzerrunner.cpp
HEADERS += \
    $$PLUGINDIR/clangstaticanalyzerrunner.h
