TEMPLATE = lib
TARGET = QmlProfiler

DEFINES += QMLPROFILER_LIBRARY

QT += network script declarative

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/analyzerbase/analyzerbase.pri)
include(../../plugins/qmlprojectmanager/qmlprojectmanager.pri)
include(../../plugins/qt4projectmanager/qt4projectmanager.pri)
include(../../plugins/remotelinux/remotelinux.pri)
include(../../libs/qmldebug/qmldebug.pri)
include(../../libs/extensionsystem/extensionsystem.pri)
include(canvas/canvas.pri)

SOURCES += \
    qmlprofilerplugin.cpp \
    qmlprofilertool.cpp \
    qmlprofilerengine.cpp \
    qmlprofilerattachdialog.cpp \
    localqmlprofilerrunner.cpp \
    remotelinuxqmlprofilerrunner.cpp \
    qmlprofilereventview.cpp \
    qmlprofilerdetailsrewriter.cpp \
    qmlprofilertraceview.cpp \
    timelinerenderer.cpp \
    qmlprofilerstatemanager.cpp \
    qv8profilerdatamodel.cpp \
    qmlprofilerdatamodel.cpp \
    qmlprofilerclientmanager.cpp \
    qmlprofilerviewmanager.cpp \
    qmlprofilerstatewidget.cpp

HEADERS += \
    qmlprofilerconstants.h \
    qmlprofiler_global.h \
    qmlprofilerplugin.h \
    qmlprofilertool.h \
    qmlprofilerengine.h \
    qmlprofilerattachdialog.h \
    abstractqmlprofilerrunner.h \
    localqmlprofilerrunner.h \
    remotelinuxqmlprofilerrunner.h \
    qmlprofilereventview.h \
    qmlprofilerdetailsrewriter.h \
    qmlprofilertraceview.h \
    timelinerenderer.h \
    qmlprofilerstatemanager.h \
    qv8profilerdatamodel.h \
    qmlprofilerdatamodel.h \
    qmlprofilerclientmanager.h \
    qmlprofilerviewmanager.h \
    qmlprofilerstatewidget.h

RESOURCES += \
    qml/qmlprofiler.qrc

OTHER_FILES += \
    qml/Detail.qml \
    qml/Label.qml \
    qml/MainView.qml \
    qml/RangeDetails.qml \
    qml/RangeMover.qml \
    qml/TimeDisplay.qml \
    qml/TimeMarks.qml \
    qml/SelectionRange.qml \
    qml/SelectionRangeDetails.qml \
    qml/Overview.qml

FORMS += \
    qmlprofilerattachdialog.ui
