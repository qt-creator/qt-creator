TARGET = QmlProfilerExtended
TEMPLATE = lib

include(../../qtcreatorplugin.pri)
include(qmlprofilerextended_dependencies.pri)

DEFINES += QMLPROFILEREXTENDED_LIBRARY

# QmlProfilerExtended files

SOURCES += qmlprofilerextendedplugin.cpp \
    scenegraphtimelinemodel.cpp

HEADERS += qmlprofilerextendedplugin.h\
        qmlprofilerextended_global.h\
        qmlprofilerextendedconstants.h \
    scenegraphtimelinemodel.h

OTHER_FILES += \
    QmlProfilerExtended.json
