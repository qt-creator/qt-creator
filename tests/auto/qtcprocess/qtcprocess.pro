QTC_LIB_DEPENDS += utils
include(../qttest.pri)

win32:DEFINES += _CRT_SECURE_NO_WARNINGS

SOURCES +=  tst_qtcprocess.cpp
