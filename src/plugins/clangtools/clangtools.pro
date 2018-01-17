include(../../qtcreatorplugin.pri)

SOURCES += \
    clangstaticanalyzerconfigwidget.cpp \
    clangstaticanalyzerdiagnosticview.cpp \
    clangstaticanalyzerprojectsettings.cpp \
    clangstaticanalyzerprojectsettingsmanager.cpp \
    clangstaticanalyzerprojectsettingswidget.cpp \
    clangstaticanalyzerruncontrol.cpp \
    clangstaticanalyzerrunner.cpp \
    clangstaticanalyzertool.cpp \
    clangtool.cpp \
    clangtoolruncontrol.cpp \
    clangtoolrunner.cpp \
    clangtoolsdiagnostic.cpp \
    clangtoolsdiagnosticmodel.cpp \
    clangtoolslogfilereader.cpp \
    clangtoolsplugin.cpp \
    clangtoolssettings.cpp \
    clangtoolsutils.cpp

HEADERS += \
    clangstaticanalyzerconfigwidget.h \
    clangstaticanalyzerdiagnosticview.h \
    clangstaticanalyzerprojectsettings.h \
    clangstaticanalyzerprojectsettingsmanager.h \
    clangstaticanalyzerprojectsettingswidget.h \
    clangstaticanalyzerruncontrol.h \
    clangstaticanalyzerrunner.h \
    clangstaticanalyzertool.h \
    clangtool.h \
    clangtoolruncontrol.h \
    clangtoolrunner.h \
    clangtools_global.h \
    clangtoolsconstants.h \
    clangtoolsdiagnostic.h \
    clangtoolsdiagnosticmodel.h \
    clangtoolslogfilereader.h \
    clangtoolsplugin.h \
    clangtoolssettings.h \
    clangtoolsutils.h

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
