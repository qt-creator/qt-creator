TARGET = PerfProfiler
TEMPLATE = lib

QT += quick quickwidgets

include(../../qtcreatorplugin.pri)
include(perfprofiler_dependencies.pri)

DEFINES += PERFPROFILER_LIBRARY

# PerfProfiler files

SOURCES += \
    perftimelinemodel.cpp \
    perfprofilerplugin.cpp \
    perfprofilertool.cpp \
    perftimelinemodelmanager.cpp \
    perfloaddialog.cpp \
    perfprofilertraceview.cpp \
    perfdatareader.cpp \
    perfprofilerruncontrol.cpp \
    perfoptionspage.cpp \
    perfsettings.cpp \
    perfconfigwidget.cpp \
    perfrunconfigurationaspect.cpp \
    perfprofilerstatisticsview.cpp \
    perfprofilerstatisticsmodel.cpp \
    perfprofilerflamegraphview.cpp \
    perfprofilerflamegraphmodel.cpp \
    perfprofilertracefile.cpp \
    perfconfigeventsmodel.cpp \
    perftimelineresourcesrenderpass.cpp \
    perfresourcecounter.cpp \
    perfprofilertracemanager.cpp \
    perftracepointdialog.cpp

HEADERS += \
    perftimelinemodel.h \
    perfprofilerplugin.h \
    perfprofilerconstants.h \
    perfprofilertool.h \
    perftimelinemodelmanager.h \
    perfloaddialog.h \
    perfprofilertraceview.h \
    perfdatareader.h \
    perfprofilerruncontrol.h \
    perfoptionspage.h \
    perfsettings.h \
    perfconfigwidget.h \
    perfrunconfigurationaspect.h \
    perfprofilerstatisticsview.h \
    perfprofilerstatisticsmodel.h \
    perfprofilerflamegraphview.h \
    perfprofilerflamegraphmodel.h \
    perfprofilertracefile.h \
    perfconfigeventsmodel.h \
    perfprofiler_global.h \
    perftimelineresourcesrenderpass.h \
    perfresourcecounter.h \
    perfprofilertracemanager.h \
    perfevent.h \
    perfeventtype.h \
    perftracepointdialog.h

RESOURCES += \
    perfprofiler.qrc

OTHER_FILES += \
    PerfProfiler.json.in

FORMS += \
    perfconfigwidget.ui \
    perfloaddialog.ui \
    perftracepointdialog.ui

equals(TEST, 1) {
include(tests/tests.pri)
}

OTHER_FILES += \
    memoryprofiling.txt
