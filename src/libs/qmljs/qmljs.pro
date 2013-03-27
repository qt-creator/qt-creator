TEMPLATE = lib
TARGET = QmlJS
DEFINES += QMLJS_BUILD_DIR QT_CREATOR

QT +=script
include(../../qtcreatorlibrary.pri)
include(qmljs_dependencies.pri)
include(qmljs-lib.pri)
