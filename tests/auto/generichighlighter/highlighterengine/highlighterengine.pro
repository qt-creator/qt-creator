QT += gui testlib

PLUGINSDIR = ../../../../src/plugins
GENERICHIGHLIGHTERDIR = $$PLUGINSDIR/texteditor/generichighlighter

SOURCES += tst_highlighterengine.cpp \
    highlightermock.cpp \
    $$GENERICHIGHLIGHTERDIR/highlighter.cpp \
    $$GENERICHIGHLIGHTERDIR/context.cpp \
    $$GENERICHIGHLIGHTERDIR/dynamicrule.cpp \
    $$GENERICHIGHLIGHTERDIR/rule.cpp \
    $$GENERICHIGHLIGHTERDIR/specificrules.cpp \
    $$GENERICHIGHLIGHTERDIR/progressdata.cpp \
    $$GENERICHIGHLIGHTERDIR/highlightdefinition.cpp \
    $$GENERICHIGHLIGHTERDIR/keywordlist.cpp \
    $$GENERICHIGHLIGHTERDIR/itemdata.cpp \
    formats.cpp

HEADERS += \
    highlightermock.h \
    basetextdocumentlayout.h \
    formats.h \
    tabsettings.h

INCLUDEPATH += $$GENERICHIGHLIGHTERDIR $$PLUGINSDIR

TARGET=tst_$$TARGET
