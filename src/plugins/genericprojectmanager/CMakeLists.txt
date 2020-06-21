if (WITH_TESTS)
  set(TST_COMPONENT CppEditor)
endif()

add_qtc_plugin(GenericProjectManager
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport TextEditor ${TST_COMPONENT}
  PLUGIN_RECOMMENDS CppTools
  SOURCES ${TEST_SOURCES}
    filesselectionwizardpage.cpp filesselectionwizardpage.h
    genericbuildconfiguration.cpp genericbuildconfiguration.h
    genericmakestep.cpp genericmakestep.h
    genericproject.cpp genericproject.h
    genericprojectconstants.h
    genericprojectfileseditor.cpp genericprojectfileseditor.h
    genericprojectplugin.cpp genericprojectplugin.h
    genericprojectwizard.cpp genericprojectwizard.h
)
