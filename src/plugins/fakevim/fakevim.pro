TEMPLATE = lib
TARGET = FakeVim

# CONFIG += single
include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/find/find.pri)

DEFINES += QT_NO_CAST_FROM_ASCII
QT += gui
SOURCES += fakevimactions.cpp \
    fakevimhandler.cpp \
    fakevimplugin.cpp
HEADERS += fakevimactions.h \
    fakevimhandler.h \
    fakevimplugin.h
FORMS += fakevimoptions.ui

equals(TEST, 1) {
    SOURCES += fakevim_test.cpp
}
