QTC_PLUGIN_DEPENDS += cpptools
include(../../qttest.pri)
DEFINES+=CPLUSPLUS_BUILD_STATIC_LIB
include($$IDE_SOURCE_TREE/src/rpath.pri)

LIBS += -L$$IDE_PLUGIN_PATH/QtProject
DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH/QtProject\\\"\"
