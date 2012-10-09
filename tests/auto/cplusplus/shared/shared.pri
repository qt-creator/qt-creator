DEFINES+=CPLUSPLUS_BUILD_STATIC_LIB
INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/cplusplus
INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/3rdparty/cplusplus
include($$IDE_SOURCE_TREE/src/plugins/cpptools/cpptools.pri)
include($$IDE_SOURCE_TREE/src/rpath.pri)

LIBS += -L$$IDE_PLUGIN_PATH/QtProject
DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH/QtProject\\\"\"
