include(../../../qttest.pri)

DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH/Nokia\\\"\"

include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs.pri)
include($$IDE_SOURCE_TREE/src/libs/extensionsystem/extensionsystem.pri)

INCLUDEPATH += \
        $$IDE_SOURCE_TREE/src/libs/3rdparty/cplusplus \
        $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/include

TARGET = tst_codemodel_basic

SOURCES += \
    tst_basic.cpp
