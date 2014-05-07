QTC_PLUGIN_DEPENDS += coreplugin
include(../qttest.pri)
include($$IDE_SOURCE_TREE/src/plugins/coreplugin/coreplugin_dependencies.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils_dependencies.pri)

LIBS *= -L$$IDE_PLUGIN_PATH

SOURCES += tst_externaltooltest.cpp \
    $$IDE_SOURCE_TREE/src/plugins/coreplugin/externaltool.cpp
HEADERS += $$IDE_SOURCE_TREE/src/plugins/coreplugin/externaltool.h
