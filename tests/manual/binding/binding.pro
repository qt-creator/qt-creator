QT =

macx:CONFIG -= app_bundle

TARGET = cplusplus0

*-g++*:QMAKE_CXXFLAGS += -fno-rtti \
    -fno-exceptions

include(../../../src/shared/cplusplus/cplusplus.pri)

unix {
    debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
    debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    release:MOC_DIR = $${OUT_PWD}/.moc/release-shared
    RCC_DIR = $${OUT_PWD}/.rcc/
    UI_DIR = $${OUT_PWD}/.uic/
}

# Input
SOURCES += main.cpp
