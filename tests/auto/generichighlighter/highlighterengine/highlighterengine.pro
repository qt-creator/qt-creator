include(../../qttest.pri)
QT += gui
PLUGINSDIR = $$IDE_SOURCE_TREE/src/plugins
GENERICHIGHLIGHTERDIR = $$PLUGINSDIR/texteditor/generichighlighter

SOURCES += \
    tst_highlighterengine.cpp \
    highlightermock.cpp \
    formats.cpp \
    texteditor/syntaxhighlighter.cpp \
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
    formats.h \
    texteditor/basetextdocumentlayout.h \
    texteditor/syntaxhighlighter.h \
    texteditor/tabsettings.h \
    $$GENERICHIGHLIGHTERDIR/highlighter.h \
    $$GENERICHIGHLIGHTERDIR/context.h \
    $$GENERICHIGHLIGHTERDIR/dynamicrule.h \
    $$GENERICHIGHLIGHTERDIR/rule.h \
    $$GENERICHIGHLIGHTERDIR/specificrules.h \
    $$GENERICHIGHLIGHTERDIR/progressdata.h \
    $$GENERICHIGHLIGHTERDIR/highlightdefinition.h \
    $$GENERICHIGHLIGHTERDIR/keywordlist.h \
    $$GENERICHIGHLIGHTERDIR/itemdata.h

INCLUDEPATH += $$PWD $$GENERICHIGHLIGHTERDIR
