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
    analyzeroutputpane.cpp

HEADERS += \
    ianalyzerengine.h \
    ianalyzertool.h \
    analyzerbase_global.h \
    analyzerconstants.h \
    analyzerplugin.h \
    analyzerruncontrol.h \
    analyzermanager.h \
    analyzersettings.h \
    analyzeroptionspage.h \
    analyzerrunconfigwidget.h \
    analyzeroutputpane.h

RESOURCES += \
    analyzerbase.qrc
