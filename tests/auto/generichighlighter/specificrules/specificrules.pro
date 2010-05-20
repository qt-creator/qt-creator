QT += testlib

PLUGINSDIR = ../../../../src/plugins

SOURCES += tst_specificrules.cpp \
    $$PLUGINSDIR/texteditor/generichighlighter/context.cpp \
    $$PLUGINSDIR/texteditor/generichighlighter/dynamicrule.cpp \
    $$PLUGINSDIR/texteditor/generichighlighter/rule.cpp \
    $$PLUGINSDIR/texteditor/generichighlighter/specificrules.cpp \
    $$PLUGINSDIR/texteditor/generichighlighter/progressdata.cpp \
    $$PLUGINSDIR/texteditor/generichighlighter/highlightdefinition.cpp \
    $$PLUGINSDIR/texteditor/generichighlighter/keywordlist.cpp \
    $$PLUGINSDIR/texteditor/generichighlighter/itemdata.cpp

INCLUDEPATH += $$PLUGINSDIR

TARGET=tst_$$TARGET
