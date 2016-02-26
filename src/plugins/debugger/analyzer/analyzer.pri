
QT += network

# AnalyzerBase files

SOURCES += \
    $$PWD/ianalyzertool.cpp \
    $$PWD/analyzerruncontrol.cpp \
    $$PWD/analyzermanager.cpp \
    $$PWD/analyzerrunconfigwidget.cpp \
    $$PWD/analyzerutils.cpp \
    $$PWD/detailederrorview.cpp \
    $$PWD/diagnosticlocation.cpp \
    $$PWD/startremotedialog.cpp

HEADERS += \
    $$PWD/ianalyzertool.h \
    $$PWD/analyzerbase_global.h \
    $$PWD/analyzerconstants.h \
    $$PWD/analyzerruncontrol.h \
    $$PWD/analyzermanager.h \
    $$PWD/analyzerstartparameters.h \
    $$PWD/analyzerrunconfigwidget.h \
    $$PWD/analyzerutils.h \
    $$PWD/detailederrorview.h \
    $$PWD/diagnosticlocation.h \
    $$PWD/startremotedialog.h \
    $$PWD/analyzericons.h

RESOURCES += \
    $$PWD/analyzerbase.qrc
