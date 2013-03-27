include(../../qttest.pri)

PLUGINSDIR = $$IDE_SOURCE_TREE/src/plugins
GENERICHIGHLIGHTERDIR = $$PLUGINSDIR/texteditor/generichighlighter

SOURCES += tst_specificrules.cpp \
    $$GENERICHIGHLIGHTERDIR/context.cpp \
    $$GENERICHIGHLIGHTERDIR/dynamicrule.cpp \
    $$GENERICHIGHLIGHTERDIR/rule.cpp \
    $$GENERICHIGHLIGHTERDIR/specificrules.cpp \
    $$GENERICHIGHLIGHTERDIR/progressdata.cpp \
    $$GENERICHIGHLIGHTERDIR/highlightdefinition.cpp \
    $$GENERICHIGHLIGHTERDIR/keywordlist.cpp \
    $$GENERICHIGHLIGHTERDIR/itemdata.cpp
