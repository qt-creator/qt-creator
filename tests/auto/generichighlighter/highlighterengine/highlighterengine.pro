QT += gui
CONFIG += qtestlib testcase

PLUGINSDIR = ../../../../src/plugins
GENERICHIGHLIGHTERDIR = $$PLUGINSDIR/texteditor/generichighlighter

SOURCES += \
    $$PWD/tst_highlighterengine.cpp \
    $$PWD/highlightermock.cpp \
    $$PWD/formats.cpp \
    $$GENERICHIGHLIGHTERDIR/highlighter.cpp \
    $$GENERICHIGHLIGHTERDIR/context.cpp \
    $$GENERICHIGHLIGHTERDIR/dynamicrule.cpp \
    $$GENERICHIGHLIGHTERDIR/rule.cpp \
    $$GENERICHIGHLIGHTERDIR/specificrules.cpp \
    $$GENERICHIGHLIGHTERDIR/progressdata.cpp \
    $$GENERICHIGHLIGHTERDIR/highlightdefinition.cpp \
    $$GENERICHIGHLIGHTERDIR/keywordlist.cpp \
    $$GENERICHIGHLIGHTERDIR/itemdata.cpp

HEADERS += \
    $$PWD/highlightermock.h \
    $$PWD/basetextdocumentlayout.h \
    $$PWD/formats.h \
    $$PWD/tabsettings.h \
    $$PWD/syntaxhighlighter.h \
    $$GENERICHIGHLIGHTERDIR/highlighter.h \
    $$GENERICHIGHLIGHTERDIR/context.h \
    $$GENERICHIGHLIGHTERDIR/dynamicrule.h \
    $$GENERICHIGHLIGHTERDIR/rule.h \
    $$GENERICHIGHLIGHTERDIR/specificrules.h \
    $$GENERICHIGHLIGHTERDIR/progressdata.h \
    $$GENERICHIGHLIGHTERDIR/highlightdefinition.h \
    $$GENERICHIGHLIGHTERDIR/keywordlist.h \
    $$GENERICHIGHLIGHTERDIR/itemdata.h

INCLUDEPATH += $$GENERICHIGHLIGHTERDIR $$PWD

TARGET=tst_$$TARGET
