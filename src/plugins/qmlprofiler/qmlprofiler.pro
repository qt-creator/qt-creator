DEFINES += QMLPROFILER_LIBRARY

QT += network qml quick quickwidgets

include(../../qtcreatorplugin.pri)

SOURCES += \
    debugmessagesmodel.cpp \
    flamegraphmodel.cpp \
    flamegraphview.cpp \
    inputeventsmodel.cpp \
    memoryusagemodel.cpp \
    pixmapcachemodel.cpp \
    qmlevent.cpp \
    qmleventlocation.cpp \
    qmleventtype.cpp \
    qmlnote.cpp \
    qmlprofileranimationsmodel.cpp \
    qmlprofilerattachdialog.cpp \
    qmlprofilerbindingloopsrenderpass.cpp \
    qmlprofilerclientmanager.cpp \
    qmlprofilerconfigwidget.cpp \
    qmlprofilerdetailsrewriter.cpp \
    qmlprofilermodelmanager.cpp \
    qmlprofilernotesmodel.cpp \
    qmlprofileroptionspage.cpp \
    qmlprofilerplugin.cpp \
    qmlprofilerrangemodel.cpp \
    qmlprofilerrunconfigurationaspect.cpp \
    qmlprofilerruncontrol.cpp \
    qmlprofilersettings.cpp \
    qmlprofilerstatemanager.cpp \
    qmlprofilerstatewidget.cpp \
    qmlprofilerstatisticsmodel.cpp \
    qmlprofilerstatisticsview.cpp \
    qmlprofilertimelinemodel.cpp \
    qmlprofilertool.cpp \
    qmlprofilertraceclient.cpp \
    qmlprofilertracefile.cpp \
    qmlprofilertraceview.cpp \
    qmlprofilerviewmanager.cpp \
    qmltypedevent.cpp \
    scenegraphtimelinemodel.cpp \
    qmlprofilertextmark.cpp

HEADERS += \
    debugmessagesmodel.h \
    flamegraphmodel.h \
    flamegraphview.h \
    inputeventsmodel.h \
    memoryusagemodel.h \
    pixmapcachemodel.h \
    qmlevent.h \
    qmleventlocation.h \
    qmleventtype.h \
    qmlnote.h \
    qmlprofiler_global.h \
    qmlprofileranimationsmodel.h \
    qmlprofilerattachdialog.h \
    qmlprofilerbindingloopsrenderpass.h \
    qmlprofilerclientmanager.h \
    qmlprofilerconfigwidget.h \
    qmlprofilerconstants.h \
    qmlprofilerdetailsrewriter.h \
    qmlprofilereventsview.h \
    qmlprofilereventtypes.h \
    qmlprofilermodelmanager.h \
    qmlprofilernotesmodel.h \
    qmlprofileroptionspage.h \
    qmlprofilerplugin.h \
    qmlprofilerrangemodel.h \
    qmlprofilerrunconfigurationaspect.h \
    qmlprofilerruncontrol.h \
    qmlprofilersettings.h \
    qmlprofilerstatemanager.h \
    qmlprofilerstatewidget.h \
    qmlprofilerstatisticsmodel.h \
    qmlprofilerstatisticsview.h \
    qmlprofilertimelinemodel.h \
    qmlprofilertool.h \
    qmlprofilertraceclient.h \
    qmlprofilertracefile.h \
    qmlprofilertraceview.h \
    qmlprofilerviewmanager.h \
    qmltypedevent.h \
    scenegraphtimelinemodel.h \
    qmlprofilertextmark.h

RESOURCES += \
    qml/qmlprofiler.qrc

FORMS += \
    qmlprofilerconfigwidget.ui

equals(TEST, 1) {
include(tests/tests.pri)
}
