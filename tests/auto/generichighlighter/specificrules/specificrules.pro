QT += testlib

PLUGINSDIR = ../../../../src/plugins

SOURCES += tst_specificrules.cpp \
    $$PLUGINSDIR/genericeditor/context.cpp \
    $$PLUGINSDIR/genericeditor/dynamicrule.cpp \
    $$PLUGINSDIR/genericeditor/rule.cpp \
    $$PLUGINSDIR/genericeditor/specificrules.cpp \
    $$PLUGINSDIR/genericeditor/progressdata.cpp \
    $$PLUGINSDIR/genericeditor/highlightdefinition.cpp \
    $$PLUGINSDIR/genericeditor/keywordlist.cpp \
    $$PLUGINSDIR/genericeditor/itemdata.cpp

INCLUDEPATH += $$PLUGINSDIR $$UTILSDIR

TARGET=tst_$$TARGET
