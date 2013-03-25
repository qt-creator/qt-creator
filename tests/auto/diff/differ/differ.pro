QTC_PLUGIN_DEPENDS += diffeditor

include(../../qttest.pri)

LIBS += -L$$IDE_PLUGIN_PATH/QtProject

SOURCES += tst_differ.cpp
