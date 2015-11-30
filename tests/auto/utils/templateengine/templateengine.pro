QTC_LIB_DEPENDS += utils
include(../../qttest.pri)

DEFINES -= QT_USE_FAST_OPERATOR_PLUS QT_USE_FAST_CONCATENATION

SOURCES += tst_templateengine.cpp
