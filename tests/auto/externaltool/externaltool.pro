IDE_BUILD_TREE = $$OUT_PWD/../../../
include(../qttest.pri)
include(../../../src/plugins/coreplugin/coreplugin.pri)
LIBS *= -L$$IDE_PLUGIN_PATH/QtProject
INCLUDEPATH *= $$IDE_SOURCE_TREE/src/plugins/coreplugin
INCLUDEPATH *= $$IDE_BUILD_TREE/src/plugins/coreplugin

SOURCES += tst_externaltooltest.cpp \
    $$IDE_SOURCE_TREE/src/plugins/coreplugin/externaltool.cpp
HEADERS += $$IDE_SOURCE_TREE/src/plugins/coreplugin/externaltool.h
