QTC_LIB_DEPENDS += qmljs
include(../../../qttest.pri)

QT += qml

PLUGIN_DIR=$$IDE_SOURCE_TREE/src/plugins/qmlprojectmanager

include($$PLUGIN_DIR/fileformat/fileformat.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils_dependencies.pri)

INCLUDEPATH += $$PLUGIN_DIR/fileformat

DEFINES += SRCDIR=\\\"$$_PRO_FILE_PWD_\\\"

TEMPLATE = app
SOURCES += tst_fileformat.cpp
