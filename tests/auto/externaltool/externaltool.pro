QTC_LIB_DEPENDS += utils
QTC_PLUGIN_DEPENDS += coreplugin
include(../qttest.pri)

SOURCES += tst_externaltooltest.cpp \
    $$IDE_SOURCE_TREE/src/plugins/coreplugin/externaltool.cpp
HEADERS += $$IDE_SOURCE_TREE/src/plugins/coreplugin/externaltool.h
