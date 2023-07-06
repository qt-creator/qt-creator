find_package(yaml-cpp QUIET MODULE)

set(CLANG_VERSION ${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH})

add_qtc_plugin(ClangTools
  CONDITION TARGET yaml-cpp
  DEPENDS yaml-cpp
  PLUGIN_DEPENDS Core Debugger CppEditor
  PLUGIN_RECOMMENDS CppEditor
  PLUGIN_TEST_DEPENDS QmakeProjectManager QbsProjectManager
  SKIP_PCH
  DEFINES
    CLANG_VERSION="${CLANG_VERSION}"
    CLANG_INCLUDE_DIR="${LLVM_LIBRARY_DIR}/clang/${CLANG_VERSION}/include"
    CLANG_BINDIR="${LLVM_TOOLS_BINARY_DIR}"
  INCLUDES ${CLANG_INCLUDE_DIRS}
  SOURCES
    clangfileinfo.h
    clangfixitsrefactoringchanges.cpp clangfixitsrefactoringchanges.h
    clangselectablefilesdialog.cpp clangselectablefilesdialog.h
    clangtool.cpp clangtool.h
    clangtoolrunner.cpp clangtoolrunner.h
    clangtools_global.h
    clangtoolstr.h
    clangtoolsconstants.h
    clangtoolsdiagnostic.cpp clangtoolsdiagnostic.h
    clangtoolsdiagnosticmodel.cpp clangtoolsdiagnosticmodel.h
    clangtoolsdiagnosticview.cpp clangtoolsdiagnosticview.h
    clangtoolslogfilereader.cpp clangtoolslogfilereader.h
    clangtoolsplugin.cpp clangtoolsplugin.h
    clangtoolsprojectsettings.cpp clangtoolsprojectsettings.h
    clangtoolsprojectsettingswidget.cpp clangtoolsprojectsettingswidget.h
    clangtoolssettings.cpp clangtoolssettings.h
    clangtoolsutils.cpp clangtoolsutils.h
    diagnosticconfigswidget.cpp diagnosticconfigswidget.h
    diagnosticmark.cpp diagnosticmark.h
    documentclangtoolrunner.cpp documentclangtoolrunner.h
    documentquickfixfactory.cpp documentquickfixfactory.h
    executableinfo.cpp executableinfo.h
    filterdialog.cpp filterdialog.h
    runsettingswidget.cpp runsettingswidget.h
    settingswidget.cpp settingswidget.h
    virtualfilesystemoverlay.cpp virtualfilesystemoverlay.h
)

extend_qtc_plugin(ClangTools
  CONDITION WITH_TESTS
  SOURCES
    clangtoolspreconfiguredsessiontests.cpp clangtoolspreconfiguredsessiontests.h
    clangtoolsunittests.cpp clangtoolsunittests.h
    clangtoolsunittests.qrc
    readexporteddiagnosticstest.cpp readexporteddiagnosticstest.h
)
