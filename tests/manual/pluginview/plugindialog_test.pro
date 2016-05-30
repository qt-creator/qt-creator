TEMPLATE = app
TARGET = plugindialog

# Input
HEADERS += plugindialog.h
SOURCES += plugindialog.cpp

QTC_LIB_DEPENDS += \
    extensionsystem \
    utils
include(../../auto/qttest.pri)
