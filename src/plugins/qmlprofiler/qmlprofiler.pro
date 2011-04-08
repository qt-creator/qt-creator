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
    qmlprofilerattachdialog.cpp \
    qmlprofilersummaryview.cpp

HEADERS += \
    qmlprofilerconstants.h \
    qmlprofiler_global.h \
    qmlprofilerplugin.h \
    qmlprofilertool.h \
    qmlprofilerengine.h \
    tracewindow.h \
    timelineview.h \
    qmlprofilerattachdialog.h \
    qmlprofilersummaryview.h

RESOURCES += \
    qml/qml.qrc

OTHER_FILES += \
    qml/Detail.qml \
    qml/Elapsed.qml \
    qml/Label.qml \
    qml/MainView.qml \
    qml/RangeDetails.qml \
    qml/RangeMover.qml \
    qml/RecordButton.qml \
    qml/ToolButton.qml \
    qml/MainView.js

FORMS += \
    qmlprofilerattachdialog.ui
