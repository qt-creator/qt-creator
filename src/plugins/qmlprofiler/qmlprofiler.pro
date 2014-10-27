DEFINES += QMLPROFILER_LIBRARY

QT += network qml quick

include(../../qtcreatorplugin.pri)

SOURCES += \
    abstracttimelinemodel.cpp \
    localqmlprofilerrunner.cpp \
    qmlprofileranimationsmodel.cpp \
    qmlprofilerattachdialog.cpp \
    qmlprofilerbasemodel.cpp \
    qmlprofilerclientmanager.cpp \
    qmlprofilerdatamodel.cpp \
    qmlprofilerdetailsrewriter.cpp \
    qmlprofilerengine.cpp \
    qmlprofilereventsmodelproxy.cpp \
    qmlprofilereventview.cpp \
    qmlprofilermodelmanager.cpp \
    qmlprofilernotesmodel.cpp \
    qmlprofilerplugin.cpp \
    qmlprofilerrangemodel.cpp \
    qmlprofilerruncontrolfactory.cpp \
    qmlprofilerstatemanager.cpp \
    qmlprofilerstatewidget.cpp \
    qmlprofilertool.cpp \
    qmlprofilertracefile.cpp \
    qmlprofilertraceview.cpp \
    qmlprofilertreeview.cpp \
    qmlprofilerviewmanager.cpp \
    qv8profilerdatamodel.cpp \
    qv8profilereventview.cpp \
    timelinemodelaggregator.cpp \
    timelinerenderer.cpp \
    timelinezoomcontrol.cpp

HEADERS += \
    abstractqmlprofilerrunner.h \
    abstracttimelinemodel.h \
    abstracttimelinemodel_p.h \
    localqmlprofilerrunner.h \
    qmlprofiler_global.h \
    qmlprofileranimationsmodel.h \
    qmlprofilerattachdialog.h \
    qmlprofilerbasemodel.h \
    qmlprofilerbasemodel_p.h \
    qmlprofilerclientmanager.h \
    qmlprofilerconstants.h \
    qmlprofilerdatamodel.h \
    qmlprofilerdetailsrewriter.h \
    qmlprofilerengine.h \
    qmlprofilereventsmodelproxy.h \
    qmlprofilereventview.h \
    qmlprofilermodelmanager.h \
    qmlprofilernotesmodel.h \
    qmlprofilerplugin.h \
    qmlprofilerrangemodel.h \
    qmlprofilerruncontrolfactory.h \
    qmlprofilerstatemanager.h \
    qmlprofilerstatewidget.h \
    qmlprofilertool.h \
    qmlprofilertracefile.h \
    qmlprofilertraceview.h \
    qmlprofilertreeview.h \
    qmlprofilerviewmanager.h \
    qv8profilerdatamodel.h \
    qv8profilereventview.h \
    timelinemodelaggregator.h \
    timelinerenderer.h \
    timelinezoomcontrol.h

RESOURCES += \
    qml/qmlprofiler.qrc

DISTFILES += \
    qml/ButtonsBar.qml \
    qml/Detail.qml \
    qml/CategoryLabel.qml \
    qml/MainView.qml \
    qml/RangeDetails.qml \
    qml/RangeMover.qml \
    qml/TimeDisplay.qml \
    qml/TimeMarks.qml \
    qml/SelectionRange.qml \
    qml/SelectionRangeDetails.qml \
    qml/Overview.qml
