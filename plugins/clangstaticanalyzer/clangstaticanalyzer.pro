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
    clangstaticanalyzerprojectsettings.cpp \
    clangstaticanalyzerprojectsettingsmanager.cpp \
    clangstaticanalyzerprojectsettingswidget.cpp \
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
    clangstaticanalyzerlicensecheck.h \
    clangstaticanalyzerlogfilereader.h \
    clangstaticanalyzerplugin.h \
    clangstaticanalyzerprojectsettings.h \
    clangstaticanalyzerprojectsettingsmanager.h \
    clangstaticanalyzerprojectsettingswidget.h \
    clangstaticanalyzerruncontrolfactory.h \
    clangstaticanalyzerruncontrol.h \
    clangstaticanalyzerrunner.h \
    clangstaticanalyzersettings.h \
    clangstaticanalyzertool.h \
    clangstaticanalyzerutils.h

FORMS += \
    clangstaticanalyzerconfigwidget.ui \
    clangstaticanalyzerprojectsettingswidget.ui

equals(TEST, 1) {
    HEADERS += clangstaticanalyzerunittests.h
    SOURCES += clangstaticanalyzerunittests.cpp
    RESOURCES += clangstaticanalyzerunittests.qrc
}

CONFIG(licensechecker): DEFINES += LICENSECHECKER

DISTFILES += \
    tests/tests.pri
