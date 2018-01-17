include(../clangtoolstest.pri)

TARGET = tst_clangstaticanalyzerrunnertest

SOURCES += \
    tst_clangstaticanalyzerrunner.cpp \
    $$PLUGINDIR/clangtoolrunner.cpp \
    $$PLUGINDIR/clangstaticanalyzerrunner.cpp
HEADERS += \
    $$PLUGINDIR/clangtoolrunner.h \
    $$PLUGINDIR/clangstaticanalyzerrunner.h
