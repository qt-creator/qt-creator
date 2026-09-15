add_qtc_plugin(CMakeProjectManager
  PLUGIN_CLASS CMakeProjectPlugin
  PLUGIN_DEPENDS Core CppEditor Debugger ProjectExplorer TextEditor QtSupport
  DEPENDS CMakeLang RstLang McpServerLib
  SOURCES
    builddirparameters.cpp builddirparameters.h
    cmake_global.h
    cmakeabstractprocessstep.cpp cmakeabstractprocessstep.h
    cmakeautocompleter.cpp cmakeautocompleter.h
    cmakeautogenparser.cpp cmakeautogenparser.h
    cmakebuildconfiguration.cpp cmakebuildconfiguration.h
    cmakebuildsystem.cpp cmakebuildsystem.h
    cmakebuildstep.cpp cmakebuildstep.h
    cmakecodestyle.cpp cmakecodestyle.h
    cmakebuildtarget.h
    cmakecommandkeywords.cpp cmakecommandkeywords.h
    cmakeconfigitem.cpp cmakeconfigitem.h
    cmakeeditor.cpp cmakeeditor.h
    cmakefilecompletionassist.cpp cmakefilecompletionassist.h
    cmakeformatter.cpp cmakeformatter.h
    cmakeindenter.cpp cmakeindenter.h
    cmakeinstallrules.cpp cmakeinstallrules.h
    cmakeinstallstep.cpp cmakeinstallstep.h
    cmakekitaspect.cpp cmakekitaspect.h
    cmakelocatorfilter.cpp cmakelocatorfilter.h
    cmakeoutline.cpp cmakeoutline.h
    cmakeoutputparser.cpp cmakeoutputparser.h
    cmakeproject.cpp cmakeproject.h
    cmakeproject.qrc
    cmakeprojectconstants.h
    cmakeprojectimporter.cpp cmakeprojectimporter.h
    cmakeprojectmanager.cpp cmakeprojectmanager.h
    cmakeprojectmanagertr.h
    cmakeprojectnodes.cpp cmakeprojectnodes.h
    cmakeprojectplugin.cpp
    cmakequickfixes.cpp cmakequickfixes.h
    cmakesemantichighlighter.cpp cmakesemantichighlighter.h
    mcptools.cpp mcptools.h
    cmakesettingspage.cpp cmakesettingspage.h
    cmakespecificsettings.cpp cmakespecificsettings.h
    cmaketool.cpp cmaketool.h
    cmaketoolmanager.cpp cmaketoolmanager.h
    cmaketoolsettingsaccessor.cpp cmaketoolsettingsaccessor.h
    cmakeusages.cpp cmakeusages.h
    cmakeutils.cpp cmakeutils.h
    conditionalsources.cpp conditionalsources.h
    configmodel.cpp configmodel.h
    configmodelitemdelegate.cpp configmodelitemdelegate.h
    fileapidataextractor.cpp fileapidataextractor.h
    fileapiparser.cpp fileapiparser.h
    fileapireader.cpp fileapireader.h
    headerdependencies.cpp headerdependencies.h
    headerdependencyscanner.cpp headerdependencyscanner.h
    headerdependencyupdater.cpp headerdependencyupdater.h
    presetsparser.cpp presetsparser.h
    presetsmacros.cpp presetsmacros.h
    projecttreehelper.cpp projecttreehelper.h
    qtinstallerpackages.cpp qtinstallerpackages.h
    targethelper.cpp targethelper.h
    testpresetshelper.cpp testpresetshelper.h
)

add_qtc_test(tst_cmake_install_rules
  CONDITION WITH_TESTS
  DEPENDS Utils ProjectExplorer CMakeProjectManager
  SOURCES
    tests/tst_cmake_install_rules.cpp
    cmakeinstallrules.cpp
)

add_qtc_test(tst_cmake_test_presets
  CONDITION WITH_TESTS
  DEPENDS Utils ProjectExplorer CMakeProjectManager
  SOURCES
    tests/tst_cmake_test_presets.cpp
    presetsparser.cpp
    presetsmacros.cpp
    testpresetshelper.cpp
)

add_qtc_test(tst_headerdependencies
  CONDITION WITH_TESTS
  DEPENDS Utils
  SOURCES
    tests/tst_headerdependencies.cpp
    headerdependencies.cpp
)

add_qtc_test(tst_headerdependencyscanner
  CONDITION WITH_TESTS
  DEPENDS Utils ProjectExplorer
  SOURCES
    tests/tst_headerdependencyscanner.cpp
    headerdependencies.cpp
    headerdependencyscanner.cpp
)

add_qtc_test(tst_headerdependencyupdater
  CONDITION WITH_TESTS
  DEPENDS Utils ProjectExplorer CppEditor Core
  SOURCES
    tests/tst_headerdependencyupdater.cpp
    headerdependencies.cpp
    headerdependencyscanner.cpp
    headerdependencyupdater.cpp
)

file(GLOB_RECURSE test_cases RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} testcases/*)
qtc_add_resources(CMakeProjectManager "testcases"
  CONDITION WITH_TESTS
  PREFIX "/cmakeprojectmanager"
  BASE "."
  FILES ${test_cases}
)
