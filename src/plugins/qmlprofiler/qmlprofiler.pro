DEFINES += QMLPROFILER_LIBRARY

QT += network qml quick quickwidgets

include(../../qtcreatorplugin.pri)

SOURCES += \
    localqmlprofilerrunner.cpp \
    qmlprofileranimationsmodel.cpp \
    qmlprofilerattachdialog.cpp \
    qmlprofilerbasemodel.cpp \
    qmlprofilerbindingloopsrenderpass.cpp \
    qmlprofilerclientmanager.cpp \
    qmlprofilerconfigwidget.cpp \
    qmlprofilerdatamodel.cpp \
    qmlprofilerdetailsrewriter.cpp \
    qmlprofilereventsmodelproxy.cpp \
    qmlprofilereventview.cpp \
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
    qmlprofilerbasemodel.h \
    qmlprofilerbasemodel_p.h \
    qmlprofilerbindingloopsrenderpass.h \
    qmlprofilerclientmanager.h \
    qmlprofilerconfigwidget.h \
    qmlprofilerconstants.h \
    qmlprofilerdatamodel.h \
    qmlprofilerdetailsrewriter.h \
    qmlprofilereventsmodelproxy.h \
    qmlprofilereventview.h \
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
