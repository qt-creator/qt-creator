DEFINES += ANALYZERBASE_LIBRARY

include(../../qtcreatorplugin.pri)

QT += network

# AnalyzerBase files

SOURCES += \
    ianalyzerengine.cpp \
    ianalyzertool.cpp \
    analyzerplugin.cpp \
    analyzerruncontrol.cpp \
    analyzermanager.cpp \
    analyzersettings.cpp \
    analyzeroptionspage.cpp \
    analyzerrunconfigwidget.cpp \
    analyzerutils.cpp \
    startremotedialog.cpp

HEADERS += \
    ianalyzerengine.h \
    ianalyzertool.h \
    analyzerbase_global.h \
    analyzerconstants.h \
    analyzerplugin.h \
    analyzerruncontrol.h \
    analyzermanager.h \
    analyzersettings.h \
    analyzerstartparameters.h \
    analyzeroptionspage.h \
    analyzerrunconfigwidget.h \
    analyzerutils.h \
    startremotedialog.h

RESOURCES += \
    analyzerbase.qrc
