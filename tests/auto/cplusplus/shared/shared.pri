QTC_PLUGIN_DEPENDS += cpptools
include(../../qttest.pri)
include($$IDE_SOURCE_TREE/src/rpath.pri)

DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH\\\"\"
