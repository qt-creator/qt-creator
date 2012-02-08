TEMPLATE = lib
TARGET = QtConcurrent
DEFINES += BUILD_QTCONCURRENT

include(../../qtcreatorlibrary.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

HEADERS += \
    qtconcurrent_global.h \
    multitask.h \
    runextensions.h
