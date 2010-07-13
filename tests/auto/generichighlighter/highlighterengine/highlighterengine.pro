QT += gui testlib

PLUGINSDIR = ../../../../src/plugins
TEXTEDITORDIR = $$PLUGINSDIR/texteditor
GENERICHIGHLIGHTERDIR = $$PLUGINSDIR/texteditor/generichighlighter

SOURCES += tst_highlighterengine.cpp \
    highlightermock.cpp \
    formats.cpp \
    $$TEXTEDITORDIR/syntaxhighlighter.cpp \
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
    highlightermock.h \
    basetextdocumentlayout.h \
    formats.h \
    tabsettings.h \
    $$TEXTEDITORDIR/syntaxhighlighter.h \
    $$GENERICHIGHLIGHTERDIR/highlighter.h \
    $$GENERICHIGHLIGHTERDIR/context.h \
    $$GENERICHIGHLIGHTERDIR/dynamicrule.h \
    $$GENERICHIGHLIGHTERDIR/rule.h \
    $$GENERICHIGHLIGHTERDIR/specificrules.h \
    $$GENERICHIGHLIGHTERDIR/progressdata.h \
    $$GENERICHIGHLIGHTERDIR/highlightdefinition.h \
    $$GENERICHIGHLIGHTERDIR/keywordlist.h \
    $$GENERICHIGHLIGHTERDIR/itemdata.h

INCLUDEPATH += $$GENERICHIGHLIGHTERDIR $$PLUGINSDIR $$TEXTEDITORDIR

TARGET=tst_$$TARGET
