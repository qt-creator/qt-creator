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
    clangstaticanalyzerpathchooser.cpp \
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
    clangstaticanalyzerpathchooser.h \
    clangstaticanalyzerplugin.h \
    clangstaticanalyzerruncontrolfactory.h \
    clangstaticanalyzerruncontrol.h \
    clangstaticanalyzerrunner.h \
    clangstaticanalyzersettings.h \
    clangstaticanalyzertool.h \
    clangstaticanalyzerutils.h

FORMS += \
    clangstaticanalyzerconfigwidget.ui

equals(TEST, 1) {
    HEADERS += clangstaticanalyzerunittests.h
    SOURCES += clangstaticanalyzerunittests.cpp
    RESOURCES += clangstaticanalyzerunittests.qrc
}

DISTFILES += \
    tests/tests.pri
