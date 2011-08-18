TEMPLATE = lib
TARGET = QmlProfiler

DEFINES += PROFILER_LIBRARY

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/analyzerbase/analyzerbase.pri)
include(../../plugins/qmlprojectmanager/qmlprojectmanager.pri)
include(../../plugins/qt4projectmanager/qt4projectmanager.pri)
include(../../plugins/remotelinux/remotelinux.pri)
include(../../libs/qmljsdebugclient/qmljsdebugclient.pri)
include(../../libs/extensionsystem/extensionsystem.pri)

QT += network script declarative

include(canvas/canvas.pri)

SOURCES += \
    qmlprofilerplugin.cpp \
    qmlprofilertool.cpp \
    qmlprofilerengine.cpp \
    tracewindow.cpp \
    timelineview.cpp \
    qmlprofilerattachdialog.cpp \
    localqmlprofilerrunner.cpp \
    codaqmlprofilerrunner.cpp \
    remotelinuxqmlprofilerrunner.cpp \
    qmlprofilereventview.cpp \
    qmlprofilerruncontrolfactory.cpp

HEADERS += \
    qmlprofilerconstants.h \
    qmlprofiler_global.h \
    qmlprofilerplugin.h \
    qmlprofilertool.h \
    qmlprofilerengine.h \
    tracewindow.h \
    timelineview.h \
    qmlprofilerattachdialog.h \
    abstractqmlprofilerrunner.h \
    localqmlprofilerrunner.h \
    codaqmlprofilerrunner.h \
    remotelinuxqmlprofilerrunner.h \
    qmlprofilereventview.h \
    qmlprofilerruncontrolfactory.h

RESOURCES += \
    qml/qml.qrc

OTHER_FILES += \
    qml/Detail.qml \
    qml/Elapsed.qml \
    qml/Label.qml \
    qml/MainView.qml \
    qml/RangeDetails.qml \
    qml/RangeMover.qml \
    qml/MainView.js \
    qml/TimeDisplay.qml \
    qml/StatusDisplay.qml

FORMS += \
    qmlprofilerattachdialog.ui
