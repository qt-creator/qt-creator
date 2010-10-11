CONFIG += qtestlib
TEMPLATE = app
CONFIG -= app_bundle

# Input
SOURCES += tst_pluginspec.cpp

RELATIVEPATH = ../..
include(../../extensionsystem_test.pri)

LIBS += -L$${OUT_PWD}/testplugin -ltest
macx {
} else:unix {
    QMAKE_RPATHDIR += $${OUT_PWD}/testplugin
}
