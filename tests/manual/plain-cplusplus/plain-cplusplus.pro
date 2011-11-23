QT -= core gui
greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
TARGET = plain-c++
DEFINES += CPLUSPLUS_WITHOUT_QT

macx {
    CONFIG -= app_bundle
    release:LIBS += -Wl,-exported_symbol -Wl,_main
}

include(../../../src/libs/3rdparty/cplusplus/cplusplus.pri)

# Input
HEADERS += Preprocessor.h
SOURCES += Preprocessor.cpp
SOURCES += main.cpp

unix {
    debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared

    debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    release:MOC_DIR = $${OUT_PWD}/.moc/release-shared

    RCC_DIR = $${OUT_PWD}/.rcc/
    UI_DIR = $${OUT_PWD}/.uic/
}
