include(qtcreator.pri)

#version check qt
!minQtVersion(4, 7, 4) {
    message("Cannot build Qt Creator with Qt version $${QT_VERSION}.")
    error("Use at least Qt 4.7.4.")
}

include(doc/doc.pri)

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src share lib/qtcreator/qtcomponents
unix:!macx:!isEmpty(copydata):SUBDIRS += bin

OTHER_FILES += dist/copyright_template.txt \
    $$files(dist/changes-*)
