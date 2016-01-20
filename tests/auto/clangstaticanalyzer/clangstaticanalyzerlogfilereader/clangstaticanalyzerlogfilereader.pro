include(../clangstaticanalyzertest.pri)

TARGET = tst_clangstaticanalyzerlogfilereader

DEFINES += SRCDIR=\\\"$$PWD/\\\"

SOURCES += \
    tst_clangstaticanalyzerlogfilereader.cpp \
    $$PLUGINDIR/clangstaticanalyzerdiagnostic.cpp \
    $$PLUGINDIR/clangstaticanalyzerlogfilereader.cpp

HEADERS += \
    $$PLUGINDIR/clangstaticanalyzerdiagnostic.h \
    $$PLUGINDIR/clangstaticanalyzerlogfilereader.h
