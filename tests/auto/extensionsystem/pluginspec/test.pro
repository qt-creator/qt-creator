TARGET = pluginspec

# Input
SOURCES += tst_pluginspec.cpp

DISTFILES += \
    $$PWD/testspecs/simplespec.json \
    $$PWD/testspecs/simplespec_experimental.json \
    $$PWD/testspecs/spec1.json \
    $$PWD/testspecs/spec2.json \
    $$PWD/testspecs/spec_wrong2.json \
    $$PWD/testspecs/spec_wrong3.json \
    $$PWD/testspecs/spec_wrong4.json \
    $$PWD/testspecs/spec_wrong5.json \
    $$PWD/testdependencies/spec1.json \
    $$PWD/testdependencies/spec2.json \
    $$PWD/testdependencies/spec3.json \
    $$PWD/testdependencies/spec4.json \
    $$PWD/testdependencies/spec5.json \
    $$PWD/testdir/spec.json

QTC_LIB_DEPENDS += extensionsystem
include(../../qttest.pri)

DEFINES += "PLUGINSPEC_DIR=\\\"$$PWD\\\""
DEFINES += "PLUGIN_DIR=\\\"$$OUT_PWD\\\""
