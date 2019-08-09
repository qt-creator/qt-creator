QT *= qml quick core

VPATH += $$PWD

INCLUDEPATH += $$PWD

DEFINES += TIMELINE_QML_PATH=\\\"$$PWD/qml/\\\"

SOURCES += \
    timelineview.cpp \
    timelinewidget.cpp \
    timelinegraphicsscene.cpp \
    timelinegraphicslayout.cpp \
    timelinepropertyitem.cpp \
    timelinesectionitem.cpp \
    timelineitem.cpp \
    timelinemovableabstractitem.cpp \
    timelineabstracttool.cpp \
    timelinemovetool.cpp \
    timelineselectiontool.cpp \
    timelineplaceholder.cpp \
    setframevaluedialog.cpp \
    timelinetoolbar.cpp \
    easingcurvedialog.cpp \
    timelinetoolbutton.cpp \
    timelinesettingsdialog.cpp \
    timelineactions.cpp \
    timelinecontext.cpp \
    timelineutils.cpp \
    timelineanimationform.cpp \
    timelineform.cpp \
    splineeditor.cpp \
    preseteditor.cpp \
    canvas.cpp \
    canvasstyledialog.cpp \
    easingcurve.cpp \
    timelinesettingsmodel.cpp \
    timelinetooldelegate.cpp \
    timelinecontrols.cpp \
    animationcurveeditormodel.cpp \
    animationcurvedialog.cpp

HEADERS += \
    timelineview.h \
    timelinewidget.h \
    timelinegraphicsscene.h \
    timelinegraphicslayout.h \
    timelinepropertyitem.h \
    timelinesectionitem.h \
    timelineitem.h \
    timelineconstants.h \
    timelinemovableabstractitem.h \
    timelineabstracttool.h \
    timelinemovetool.h \
    timelineselectiontool.h \
    timelineplaceholder.h \
    timelineicons.h \
    timelinetoolbar.h \
    setframevaluedialog.h \
    easingcurvedialog.h \
    timelinetoolbutton.h \
    timelinesettingsdialog.h \
    timelineactions.h \
    timelinecontext.h \
    timelineutils.h \
    timelineanimationform.h \
    timelineform.h \
    splineeditor.h \
    preseteditor.h \
    canvas.h \
    canvasstyledialog.h \
    easingcurve.h \
    timelinesettingsmodel.h \
    timelinecontrols.h \
    animationcurveeditormodel.h \
    animationcurvedialog.h

RESOURCES += \
    timeline.qrc

FORMS += \
    setframevaluedialog.ui \
    timelinesettingsdialog.ui \
    timelineanimationform.ui \
    timelineform.ui
