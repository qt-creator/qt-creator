#version check qt
contains(QT_VERSION, ^4\\.[0-6]\\..*) {
    message("Cannot build Qt Creator with Qt version $${QT_VERSION}.")
    error("Use at least Qt 4.7.")
}

include(qtcreator.pri)
include(doc/doc.pri)

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src share

OTHER_FILES += dist/copyright_template.txt \
    dist/changes-1.1.0 \
    dist/changes-1.1.1 \
    dist/changes-1.2.0 \
    dist/changes-1.2.1 \
    dist/changes-1.3.0 \
    dist/changes-1.3.1 \
    dist/changes-2.0.0 \
    dist/changes-2.0.1 \
    dist/changes-2.1.0 \
    dist/changes-2.2.0 \
    dist/changes-2.3.0
