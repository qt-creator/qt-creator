#version check qt
contains(QT_VERSION, ^4\.[0-6]\..*) {
    message("Cannot build Qt Creator with Qt version $${QT_VERSION}.")
    error("Use at least Qt 4.7.")
}

include(qtcreator.pri)
include(doc/doc.pri)

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src share
unix:!macx:!isEmpty(copydata):SUBDIRS += bin
