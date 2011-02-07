TEMPLATE = lib
TARGET = Macros

DEFINES += MACROS_LIBRARY

include(../../qtcreatorplugin.pri)
include(macros_dependencies.pri)

HEADERS += macrosplugin.h \
    macros_global.h \
    macrosconstants.h \
    macro.h \
    macroevent.h \
    macromanager.h \
    macrooptionspage.h \
    macrooptionswidget.h \
    macrolocatorfilter.h \
    savedialog.h \
    macrotextfind.h \
    texteditormacrohandler.h \
    actionmacrohandler.h \
    findmacrohandler.h \
    imacrohandler.h

SOURCES += macrosplugin.cpp \
    macro.cpp \
    macroevent.cpp \
    macromanager.cpp \
    macrooptionswidget.cpp \
    macrooptionspage.cpp \
    macrolocatorfilter.cpp \
    savedialog.cpp \
    macrotextfind.cpp \
    texteditormacrohandler.cpp \
    actionmacrohandler.cpp \
    findmacrohandler.cpp \
    imacrohandler.cpp

RESOURCES += macros.qrc

FORMS += \
    macrooptionswidget.ui \
    savedialog.ui
