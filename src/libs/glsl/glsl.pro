TEMPLATE = lib
TARGET = GLSL
DEFINES += \
    GLSL_BUILD_LIB \
    QT_CREATOR

include(../../qtcreatorlibrary.pri)
include(glsl-lib.pri)
include(../utils/utils.pri)
