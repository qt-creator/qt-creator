DEFINES+=CPLUSPLUS_BUILD_STATIC_LIB
include(../../../../qtcreator.pri)
INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/cplusplus
INCLUDEPATH += $$IDE_SOURCE_TREE/src/shared/cplusplus
include($$IDE_SOURCE_TREE/src/plugins/cpptools/cpptools.pri)
LIBS += -L$$IDE_PLUGIN_PATH/Nokia
