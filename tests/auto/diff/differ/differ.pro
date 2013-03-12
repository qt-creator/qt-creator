include(../../qttest.pri)

include($$IDE_SOURCE_TREE/src/plugins/diffeditor/diffeditor.pri)

LIBS += -L$$IDE_PLUGIN_PATH/QtProject

SOURCES += tst_differ.cpp

INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins $$IDE_SOURCE_TREE/src/libs

