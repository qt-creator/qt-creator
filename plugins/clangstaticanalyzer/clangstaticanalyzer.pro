TARGET = ClangStaticAnalyzer
TEMPLATE = lib

include(../../qtcreatorplugin.pri)
include(clangstaticanalyzer_dependencies.pri)

SOURCES += \
    clangstaticanalyzerconfigwidget.cpp \
    clangstaticanalyzerdiagnostic.cpp \
    clangstaticanalyzerdiagnosticmodel.cpp \
    clangstaticanalyzerdiagnosticview.cpp \
    clangstaticanalyzerlogfilereader.cpp \
    clangstaticanalyzerplugin.cpp \
    clangstaticanalyzerruncontrol.cpp \
    clangstaticanalyzerruncontrolfactory.cpp \
    clangstaticanalyzerrunner.cpp \
    clangstaticanalyzersettings.cpp \
    clangstaticanalyzertool.cpp \
    clangstaticanalyzerutils.cpp

HEADERS += \
    clangstaticanalyzerconfigwidget.h \
    clangstaticanalyzerconstants.h \
    clangstaticanalyzerdiagnostic.h \
    clangstaticanalyzerdiagnosticmodel.h \
    clangstaticanalyzerdiagnosticview.h \
    clangstaticanalyzer_global.h \
    clangstaticanalyzerlogfilereader.h \
    clangstaticanalyzerplugin.h \
    clangstaticanalyzerruncontrolfactory.h \
    clangstaticanalyzerruncontrol.h \
    clangstaticanalyzerrunner.h \
    clangstaticanalyzersettings.h \
    clangstaticanalyzertool.h \
    clangstaticanalyzerutils.h

FORMS += \
    clangstaticanalyzerconfigwidget.ui

DISTFILES += \
    tests/tests.pri
