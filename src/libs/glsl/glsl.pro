TEMPLATE = lib
TARGET = GLSL
DEFINES += \
    GLSL_BUILD_LIB \
    QT_CREATOR \
    QT_NO_CAST_FROM_ASCII

include(../../qtcreatorlibrary.pri)
include(glsl-lib.pri)
include(../utils/utils.pri)
