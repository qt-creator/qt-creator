TEMPLATE = lib
TARGET = BinEditor

include(bineditor_dependencies.pri)

HEADERS += bineditorplugin.h \
        bineditor.h \
        bineditorconstants.h

SOURCES += bineditorplugin.cpp \
        bineditor.cpp

RESOURCES += bineditor.qrc
