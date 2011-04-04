TEMPLATE = lib
TARGET = QmlProfiler

DEFINES += PROFILER_LIBRARY

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/analyzerbase/analyzerbase.pri)


QT += network script declarative

include(canvas/canvas.pri)
#include($$QMLJSDEBUGGER_PATH/qmljsdebugger-lib.pri)

SOURCES += \
    qmlprofilerplugin.cpp \
    qmlprofilertool.cpp \
    qmlprofilerengine.cpp \
    tracewindow.cpp \
    timelineview.cpp \
    qmlprofilerattachdialog.cpp

HEADERS += \
    qmlprofilerconstants.h \
    qmlprofiler_global.h \
    qmlprofilerplugin.h \
    qmlprofilertool.h \
    qmlprofilerengine.h \
    tracewindow.h \
    timelineview.h \
    qmlprofilerattachdialog.h

RESOURCES += \
    qml/qml.qrc

OTHER_FILES += \
    Detail.qml \
    Elapsed.qml \
    Label.qml \
    MainView.qml \
    RangeDetails.qml \
    RangeMover.qml \
    RecordButton.qml \
    ToolButton.qml \
    MainView.js

FORMS += \
    qmlprofilerattachdialog.ui
