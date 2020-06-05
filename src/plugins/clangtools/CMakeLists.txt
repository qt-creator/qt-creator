if (WITH_TESTS)
  set(TST_COMPONENT QmakeProjectManager)
endif()

find_package(yaml-cpp QUIET MODULE)

add_qtc_plugin(ClangTools
  CONDITION TARGET yaml-cpp
  DEPENDS ClangSupport yaml-cpp
  PLUGIN_DEPENDS Core Debugger CppTools ${TST_COMPONENT}
  PLUGIN_RECOMMENDS CppEditor
  INCLUDES ${CLANG_INCLUDE_DIRS}
  SOURCES
    clangfileinfo.h
    clangfixitsrefactoringchanges.cpp clangfixitsrefactoringchanges.h
    clangselectablefilesdialog.cpp clangselectablefilesdialog.h clangselectablefilesdialog.ui
    clangtidyclazyrunner.cpp clangtidyclazyrunner.h
    clangtool.cpp clangtool.h
    clangtoolruncontrol.cpp clangtoolruncontrol.h
    clangtoolrunner.cpp clangtoolrunner.h
    clangtools_global.h
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
    clazychecks.ui
    diagnosticconfigswidget.cpp diagnosticconfigswidget.h
    executableinfo.cpp executableinfo.h
    filterdialog.cpp filterdialog.h filterdialog.ui
    runsettingswidget.cpp runsettingswidget.h runsettingswidget.ui
    settingswidget.cpp settingswidget.h settingswidget.ui
    tidychecks.ui
)

extend_qtc_plugin(ClangTools
  CONDITION WITH_TESTS
  SOURCES
    clangtoolspreconfiguredsessiontests.cpp clangtoolspreconfiguredsessiontests.h
    clangtoolsunittests.cpp clangtoolsunittests.h
    clangtoolsunittests.qrc
)
