DEFINES += QMLPROFILER_LIBRARY

QT += network qml quick quickwidgets

include(../../qtcreatorplugin.pri)

SOURCES += \
    localqmlprofilerrunner.cpp \
    qmlprofileranimationsmodel.cpp \
    qmlprofilerattachdialog.cpp \
    qmlprofilerbindingloopsrenderpass.cpp \
    qmlprofilerclientmanager.cpp \
    qmlprofilerconfigwidget.cpp \
    qmlprofilerdatamodel.cpp \
    qmlprofilerdetailsrewriter.cpp \
    qmlprofilermodelmanager.cpp \
    qmlprofilernotesmodel.cpp \
    qmlprofileroptionspage.cpp \
    qmlprofilerplugin.cpp \
    qmlprofilerrangemodel.cpp \
    qmlprofilerrunconfigurationaspect.cpp \
    qmlprofilerruncontrol.cpp \
    qmlprofilerruncontrolfactory.cpp \
    qmlprofilersettings.cpp \
    qmlprofilerstatemanager.cpp \
    qmlprofilerstatewidget.cpp \
    qmlprofilerstatisticsmodel.cpp \
    qmlprofilerstatisticsview.cpp \
    qmlprofilertimelinemodel.cpp \
    qmlprofilertimelinemodelfactory.cpp \
    qmlprofilertool.cpp \
    qmlprofilertracefile.cpp \
    qmlprofilertraceview.cpp \
    qmlprofilerviewmanager.cpp

HEADERS += \
    localqmlprofilerrunner.h \
    qmlprofiler_global.h \
    qmlprofileranimationsmodel.h \
    qmlprofilerattachdialog.h \
    qmlprofilerbindingloopsrenderpass.h \
    qmlprofilerclientmanager.h \
    qmlprofilerconfigwidget.h \
    qmlprofilerconstants.h \
    qmlprofilerdatamodel.h \
    qmlprofilerdetailsrewriter.h \
    qmlprofilereventsview.h \
    qmlprofilermodelmanager.h \
    qmlprofilernotesmodel.h \
    qmlprofileroptionspage.h \
    qmlprofilerplugin.h \
    qmlprofilerrangemodel.h \
    qmlprofilerrunconfigurationaspect.h \
    qmlprofilerruncontrol.h \
    qmlprofilerruncontrolfactory.h \
    qmlprofilersettings.h \
    qmlprofilerstatemanager.h \
    qmlprofilerstatewidget.h \
    qmlprofilerstatisticsmodel.h \
    qmlprofilerstatisticsview.h \
    qmlprofilertimelinemodel.h \
    qmlprofilertimelinemodelfactory.h \
    qmlprofilertool.h \
    qmlprofilertracefile.h \
    qmlprofilertraceview.h \
    qmlprofilerviewmanager.h

RESOURCES += \
    qml/qmlprofiler.qrc

DISTFILES += \
    qml/bindingloops.frag \
    qml/bindingloops.vert

FORMS += \
    qmlprofilerconfigwidget.ui
