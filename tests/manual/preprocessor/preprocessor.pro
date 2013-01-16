QT = core
macx:CONFIG -= app_bundle
TARGET = pp

include(../../auto/qttest.pri)

include($$IDE_SOURCE_TREE/src/libs/cplusplus/cplusplus.pri)
include($$IDE_SOURCE_TREE/src/libs/languageutils/languageutils.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/libs/3rdparty/botan/botan.pri)

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
