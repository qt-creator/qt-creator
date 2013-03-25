TARGET = pluginspec

# Input
SOURCES += tst_pluginspec.cpp

OTHER_FILES += \
    $$PWD/testspecs/simplespec.xml \
    $$PWD/testspecs/simplespec_experimental.xml \
    $$PWD/testspecs/spec1.xml \
    $$PWD/testspecs/spec2.xml \
    $$PWD/testspecs/spec_wrong1.xml \
    $$PWD/testspecs/spec_wrong2.xml \
    $$PWD/testspecs/spec_wrong3.xml \
    $$PWD/testspecs/spec_wrong4.xml \
    $$PWD/testspecs/spec_wrong5.xml \
    $$PWD/testdependencies/spec1.xml \
    $$PWD/testdependencies/spec2.xml \
    $$PWD/testdependencies/spec3.xml \
    $$PWD/testdependencies/spec4.xml \
    $$PWD/testdependencies/spec5.xml \
    $$PWD/testdir/spec.xml

QTC_LIB_DEPENDS += extensionsystem
include(../../qttest.pri)

DEFINES += "PLUGINSPEC_DIR=\\\"$$PWD\\\""

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../copy.pri)
