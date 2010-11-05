QT = core gui
macx:CONFIG -= app_bundle
TARGET = qml-ast2dot

include(../../../src/libs/qmljs/qmljs-lib.pri)

SOURCES += ../../../src/libs/utils/changeset.cpp
HEADERS += ../../../src/libs/utils/changeset.h

# Input
SOURCES += main.cpp

unix {
    debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared

    debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    release:MOC_DIR = $${OUT_PWD}/.moc/release-shared

    RCC_DIR = $${OUT_PWD}/.rcc/
    UI_DIR = $${OUT_PWD}/.uic/
}
