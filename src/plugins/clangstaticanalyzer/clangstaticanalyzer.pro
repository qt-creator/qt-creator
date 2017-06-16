include(../../qtcreatorplugin.pri)

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
    clangstaticanalyzerprojectsettings.h \
    clangstaticanalyzerprojectsettingsmanager.h \
    clangstaticanalyzerprojectsettingswidget.h \
    clangstaticanalyzerruncontrol.h \
    clangstaticanalyzerrunner.h \
    clangstaticanalyzersettings.h \
    clangstaticanalyzertool.h \
    clangstaticanalyzerutils.h

FORMS += \
    clangstaticanalyzerconfigwidget.ui \
    clangstaticanalyzerprojectsettingswidget.ui

equals(TEST, 1) {
    HEADERS += \
        clangstaticanalyzerpreconfiguredsessiontests.h \
        clangstaticanalyzerunittests.h

    SOURCES += \
        clangstaticanalyzerpreconfiguredsessiontests.cpp \
        clangstaticanalyzerunittests.cpp

    RESOURCES += clangstaticanalyzerunittests.qrc
}

DISTFILES += \
    tests/tests.pri \
    $${IDE_SOURCE_TREE}/doc/src/analyze/creator-clang-static-analyzer.qdoc
