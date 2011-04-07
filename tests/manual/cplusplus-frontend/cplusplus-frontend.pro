QT = core gui
macx:CONFIG -= app_bundle
TARGET = cplusplus0
include(../../../qtcreator.pri)
include($$IDE_SOURCE_TREE/src/libs/cplusplus/cplusplus.pri)
include($$IDE_SOURCE_TREE/src/libs/languageutils/languageutils.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

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
