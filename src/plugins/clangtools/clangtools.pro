include(../../qtcreatorplugin.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

requires(!isEmpty(LLVM_VERSION))

LIBS += $$LIBCLANG_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH

SOURCES += \
    clangfixitsrefactoringchanges.cpp \
    clangselectablefilesdialog.cpp \
    clangtoolsdiagnosticview.cpp \
    clangtoolsprojectsettingswidget.cpp \
    clangtidyclazyruncontrol.cpp \
    clangtidyclazyrunner.cpp \
    clangtidyclazytool.cpp \
    clangtool.cpp \
    clangtoolruncontrol.cpp \
    clangtoolrunner.cpp \
    clangtoolsbasicsettings.cpp \
    clangtoolsdiagnostic.cpp \
    clangtoolsdiagnosticmodel.cpp \
    clangtoolslogfilereader.cpp \
    clangtoolsplugin.cpp \
    clangtoolsprojectsettings.cpp \
    clangtoolssettings.cpp \
    clangtoolsutils.cpp \
    clangtoolsconfigwidget.cpp

HEADERS += \
    clangfileinfo.h \
    clangfixitsrefactoringchanges.h \
    clangselectablefilesdialog.h \
    clangtoolsdiagnosticview.h \
    clangtoolsprojectsettingswidget.h \
    clangtidyclazyruncontrol.h \
    clangtidyclazyrunner.h \
    clangtidyclazytool.h \
    clangtool.h \
    clangtoolruncontrol.h \
    clangtoolrunner.h \
    clangtools_global.h \
    clangtoolsbasicsettings.h \
    clangtoolsconstants.h \
    clangtoolsdiagnostic.h \
    clangtoolsdiagnosticmodel.h \
    clangtoolslogfilereader.h \
    clangtoolsplugin.h \
    clangtoolsprojectsettings.h \
    clangtoolssettings.h \
    clangtoolsutils.h \
    clangtoolsconfigwidget.h

FORMS += \
    clangtoolsprojectsettingswidget.ui \
    clangtoolsconfigwidget.ui \
    clangselectablefilesdialog.ui \
    clangtoolsbasicsettings.ui

equals(TEST, 1) {
    HEADERS += \
        clangtoolspreconfiguredsessiontests.h \
        clangtoolsunittests.h

    SOURCES += \
        clangtoolspreconfiguredsessiontests.cpp \
        clangtoolsunittests.cpp

    RESOURCES += clangtoolsunittests.qrc
}

DISTFILES += \
    tests/tests.pri \
    $${IDE_SOURCE_TREE}/doc/src/analyze/creator-clang-static-analyzer.qdoc
