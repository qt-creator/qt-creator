include(../../../../../src/plugins/qmldesigner/config.pri)
QT += testlib \
    script \
    declarative
DESTDIR = $$DESIGNER_BINARY_DIRECTORY
include(../../../../../src/plugins/qmldesigner/core/core.pri)
include(../../../../../src/shared/qmljs/qmljs.pri)
HEADERS+=../../../../../src/libs/utils/changeset.h
SOURCES+=../../../../../src/libs/utils/changeset.cpp
INCLUDEPATH+=../../../../../src/libs
DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB

DEPENDPATH += ..
DEPENDPATH += ../../../../../src/plugins/qmldesigner/core/include

TARGET = tst_qmldesigner_core
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += testcore.cpp \
    ../testview.cpp \
    testrewriterview.cpp
HEADERS += testcore.h \
    ../testview.h \
    testrewriterview.h
RESOURCES += ../data/testfiles.qrc
