include(../../qtcreatorplugin.pri)

QT += qml quick quickwidgets

DEFINES += QMLPROFILEREXTENSION_LIBRARY

# QmlProfilerExtension files

SOURCES += qmlprofilerextensionplugin.cpp \
        scenegraphtimelinemodel.cpp \
        pixmapcachemodel.cpp \
        memoryusagemodel.cpp \
        inputeventsmodel.cpp \
        debugmessagesmodel.cpp \
        flamegraphmodel.cpp \
        flamegraphview.cpp \
        flamegraph.cpp

HEADERS += qmlprofilerextensionplugin.h \
        qmlprofilerextension_global.h \
        qmlprofilerextensionconstants.h \
        scenegraphtimelinemodel.h \
        pixmapcachemodel.h \
        memoryusagemodel.h \
        inputeventsmodel.h \
        debugmessagesmodel.h \
        flamegraphmodel.h \
        flamegraphview.h \
        flamegraph.h

OTHER_FILES += \
    QmlProfilerExtension.json.in

RESOURCES += \
    flamegraph.qrc
