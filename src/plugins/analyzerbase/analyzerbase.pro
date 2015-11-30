DEFINES += ANALYZERBASE_LIBRARY

include(../../qtcreatorplugin.pri)

QT += network

# AnalyzerBase files

SOURCES += \
    ianalyzertool.cpp \
    analyzerplugin.cpp \
    analyzerruncontrol.cpp \
    analyzermanager.cpp \
    analyzerrunconfigwidget.cpp \
    analyzerutils.cpp \
    detailederrorview.cpp \
    diagnosticlocation.cpp \
    startremotedialog.cpp

HEADERS += \
    ianalyzertool.h \
    analyzerbase_global.h \
    analyzerconstants.h \
    analyzerplugin.h \
    analyzerruncontrol.h \
    analyzermanager.h \
    analyzerstartparameters.h \
    analyzerrunconfigwidget.h \
    analyzerutils.h \
    detailederrorview.h \
    diagnosticlocation.h \
    startremotedialog.h \
    analyzericons.h

RESOURCES += \
    analyzerbase.qrc
