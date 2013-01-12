TEMPLATE = lib
TARGET = AnalyzerBase

DEFINES += ANALYZER_LIBRARY

include(../../qtcreatorplugin.pri)
include(analyzerbase_dependencies.pri)

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
    startremotedialog.cpp \
    analyzerruncontrolfactory.cpp

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
    startremotedialog.h \
    analyzerruncontrolfactory.h

RESOURCES += \
    analyzerbase.qrc
