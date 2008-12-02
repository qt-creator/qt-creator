CONFIG += qtestlib
TEMPLATE = app
CONFIG -= app_bundle
DESTDIR = $${PWD}
# Input
SOURCES += tst_pluginspec.cpp

include(../../extensionsystem_test.pri)

LIBS += -L$${PWD}/testplugin -ltest
macx {
} else:unix {
    QMAKE_RPATHDIR += $${PWD}/testplugin
}
