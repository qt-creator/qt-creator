include(../../qtcreatorplugin.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

requires(!isEmpty(LLVM_VERSION))

LIBS += $$LIBCLANG_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

SOURCES += \
    clangstaticanalyzerdiagnosticview.cpp \
    clangstaticanalyzerprojectsettings.cpp \
    clangstaticanalyzerprojectsettingsmanager.cpp \
    clangstaticanalyzerprojectsettingswidget.cpp \
    clangstaticanalyzerruncontrol.cpp \
    clangstaticanalyzerrunner.cpp \
    clangstaticanalyzertool.cpp \
    clangtidyclazyruncontrol.cpp \
    clangtidyclazyrunner.cpp \
    clangtidyclazytool.cpp \
    clangtool.cpp \
    clangtoolruncontrol.cpp \
    clangtoolrunner.cpp \
    clangtoolsdiagnostic.cpp \
    clangtoolsdiagnosticmodel.cpp \
    clangtoolslogfilereader.cpp \
    clangtoolsplugin.cpp \
    clangtoolssettings.cpp \
    clangtoolsutils.cpp \
    clangtoolsconfigwidget.cpp

HEADERS += \
    clangstaticanalyzerdiagnosticview.h \
    clangstaticanalyzerprojectsettings.h \
    clangstaticanalyzerprojectsettingsmanager.h \
    clangstaticanalyzerprojectsettingswidget.h \
    clangstaticanalyzerruncontrol.h \
    clangstaticanalyzerrunner.h \
    clangstaticanalyzertool.h \
    clangtidyclazyruncontrol.h \
    clangtidyclazyrunner.h \
    clangtidyclazytool.h \
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
    clangtoolsutils.h \
    clangtoolsconfigwidget.h

FORMS += \
    clangstaticanalyzerprojectsettingswidget.ui \
    clangtoolsconfigwidget.ui

equals(TEST, 1) {
    HEADERS += \
        clangstaticanalyzerpreconfiguredsessiontests.h \
        clangtoolsunittests.h

    SOURCES += \
        clangstaticanalyzerpreconfiguredsessiontests.cpp \
        clangtoolsunittests.cpp

    RESOURCES += clangtoolsunittests.qrc
}

DISTFILES += \
    tests/tests.pri \
    $${IDE_SOURCE_TREE}/doc/src/analyze/creator-clang-static-analyzer.qdoc
