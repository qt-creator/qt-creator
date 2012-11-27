TEMPLATE = lib
TARGET = QmlJS
DEFINES += QMLJS_BUILD_DIR QT_CREATOR QT_NO_CAST_FROM_ASCII

QT +=script
include(../../qtcreatorlibrary.pri)
include(qmljs-lib.pri)
include(../utils/utils.pri)
include(../languageutils/languageutils.pri)
