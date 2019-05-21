if (WITH_TESTS)
  set(TST_COMPONENT QmakeProjectManager)
endif()

add_qtc_plugin(ClangTools
  CONDITION TARGET libclang
  DEPENDS ClangSupport libclang
  PLUGIN_DEPENDS Core Debugger CppTools ${TST_COMPONENT}
  INCLUDES ${CLANG_INCLUDE_DIRS}
  SOURCES
    clangfileinfo.h
    clangfixitsrefactoringchanges.cpp clangfixitsrefactoringchanges.h
    clangselectablefilesdialog.cpp clangselectablefilesdialog.h clangselectablefilesdialog.ui
    clangtidyclazyruncontrol.cpp clangtidyclazyruncontrol.h
    clangtidyclazyrunner.cpp clangtidyclazyrunner.h
    clangtidyclazytool.cpp clangtidyclazytool.h
    clangtool.cpp clangtool.h
    clangtoolruncontrol.cpp clangtoolruncontrol.h
    clangtoolrunner.cpp clangtoolrunner.h
    clangtools_global.h
    clangtoolsbasicsettings.cpp clangtoolsbasicsettings.h clangtoolsbasicsettings.ui
    clangtoolsconfigwidget.cpp clangtoolsconfigwidget.h clangtoolsconfigwidget.ui
    clangtoolsconstants.h
    clangtoolsdiagnostic.cpp clangtoolsdiagnostic.h
    clangtoolsdiagnosticmodel.cpp clangtoolsdiagnosticmodel.h
    clangtoolsdiagnosticview.cpp clangtoolsdiagnosticview.h
    clangtoolslogfilereader.cpp clangtoolslogfilereader.h
    clangtoolsplugin.cpp clangtoolsplugin.h
    clangtoolsprojectsettings.cpp clangtoolsprojectsettings.h
    clangtoolsprojectsettingswidget.cpp clangtoolsprojectsettingswidget.h clangtoolsprojectsettingswidget.ui
    clangtoolssettings.cpp clangtoolssettings.h
    clangtoolsutils.cpp clangtoolsutils.h
)

extend_qtc_plugin(ClangTools
  CONDITION WITH_TESTS
  SOURCES
    clangtoolspreconfiguredsessiontests.cpp clangtoolspreconfiguredsessiontests.h
    clangtoolsunittests.cpp clangtoolsunittests.h
    clangtoolsunittests.qrc
)
