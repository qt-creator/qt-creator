DEFINES += QMLPROFILER_LIBRARY

QT += network qml quick

include(../../qtcreatorplugin.pri)

SOURCES += \
    qmlprofilerplugin.cpp \
    qmlprofilertool.cpp \
    qmlprofilerengine.cpp \
    qmlprofilerattachdialog.cpp \
    localqmlprofilerrunner.cpp \
    qmlprofilereventview.cpp \
    qv8profilereventview.cpp \
    qmlprofilerdetailsrewriter.cpp \
    qmlprofilertraceview.cpp \
    timelinerenderer.cpp \
    qmlprofilerstatemanager.cpp \
    qv8profilerdatamodel.cpp \
    qmlprofilerclientmanager.cpp \
    qmlprofilerviewmanager.cpp \
    qmlprofilerstatewidget.cpp \
    qmlprofilerruncontrolfactory.cpp \
    qmlprofilermodelmanager.cpp \
    qmlprofilereventsmodelproxy.cpp \
    qmlprofilertimelinemodelproxy.cpp \
    qmlprofilertreeview.cpp \
    qmlprofilertracefile.cpp \
    abstracttimelinemodel.cpp \
    timelinemodelaggregator.cpp \
    qmlprofilerpainteventsmodelproxy.cpp \
    sortedtimelinemodel.cpp \
    qmlprofilerbasemodel.cpp \
    singlecategorytimelinemodel.cpp \
    qmlprofilerdatamodel.cpp

HEADERS += \
    qmlprofilerconstants.h \
    qmlprofiler_global.h \
    qmlprofilerplugin.h \
    qmlprofilertool.h \
    qmlprofilerengine.h \
    qmlprofilerattachdialog.h \
    abstractqmlprofilerrunner.h \
    localqmlprofilerrunner.h \
    qmlprofilereventview.h \
    qv8profilereventview.h \
    qmlprofilerdetailsrewriter.h \
    qmlprofilertraceview.h \
    timelinerenderer.h \
    qmlprofilerstatemanager.h \
    qv8profilerdatamodel.h \
    qmlprofilerclientmanager.h \
    qmlprofilerviewmanager.h \
    qmlprofilerstatewidget.h \
    qmlprofilerruncontrolfactory.h \
    qmlprofilermodelmanager.h \
    qmlprofilereventsmodelproxy.h \
    qmlprofilertimelinemodelproxy.h \
    qmlprofilertreeview.h \
    qmlprofilertracefile.h \
    abstracttimelinemodel.h \
    timelinemodelaggregator.h \
    qmlprofilerpainteventsmodelproxy.h \
    sortedtimelinemodel.h \
    qmlprofilerbasemodel.h \
    abstracttimelinemodel_p.h \
    singlecategorytimelinemodel.h \
    singlecategorytimelinemodel_p.h \
    qmlprofilerdatamodel.h \
    qmlprofilerbasemodel_p.h

RESOURCES += \
    qml/qmlprofiler.qrc

OTHER_FILES += \
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
