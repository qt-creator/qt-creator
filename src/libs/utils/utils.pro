TEMPLATE = lib
TARGET = Utils
QT += gui \
    network

include(../../qtcreatorlibrary.pri)
include(utils_dependencies.pri)

include(utils-lib.pri)

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += concurrent
} else {
#   Needed for QtCore/private/qwineventnotifier_p.h
    win32:include(../../private_headers.pri)
}

HEADERS += \
    proxyaction.h

SOURCES += \
    proxyaction.cpp

win32: LIBS += -lUser32
