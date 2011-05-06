TEMPLATE = lib
TARGET = AnalyzerBase

DEFINES += ANALYZER_LIBRARY

include(../../qtcreatorplugin.pri)
include(analyzerbase_dependencies.pri)

QT += network

# AnalyzerBase files

SOURCES += \
    ianalyzerengine.cpp \
    ianalyzeroutputpaneadapter.cpp \
    ianalyzertool.cpp \
    analyzerplugin.cpp \
    analyzerruncontrol.cpp \
    analyzerruncontrolfactory.cpp \
    analyzermanager.cpp \
    analyzersettings.cpp \
    analyzeroptionspage.cpp \
    analyzerrunconfigwidget.cpp \
    analyzeroutputpane.cpp \
    analyzerutils.cpp \
    startremotedialog.cpp

HEADERS += \
    ianalyzerengine.h \
    ianalyzeroutputpaneadapter.h \
    ianalyzertool.h \
    analyzerbase_global.h \
    analyzerconstants.h \
    analyzerplugin.h \
    analyzerruncontrol.h \
    analyzerruncontrolfactory.h \
    analyzermanager.h \
    analyzersettings.h \
    analyzerstartparameters.h \
    analyzeroptionspage.h \
    analyzerrunconfigwidget.h \
    analyzeroutputpane.h \
    analyzerutils.h \
    startremotedialog.h

FORMS += \
    startremotedialog.ui

RESOURCES += \
    analyzerbase.qrc
