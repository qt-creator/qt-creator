if (WITH_TESTS)
  set(TST_COMPONENT CppEditor)
endif()

add_qtc_plugin(GenericProjectManager
  PLUGIN_DEPENDS Core CppTools ProjectExplorer QtSupport TextEditor ${TST_COMPONENT}
  SOURCES ${TEST_SOURCES}
    filesselectionwizardpage.cpp filesselectionwizardpage.h
    genericbuildconfiguration.cpp genericbuildconfiguration.h
    genericmakestep.cpp genericmakestep.h
    genericproject.cpp genericproject.h
    genericprojectconstants.h
    genericprojectfileseditor.cpp genericprojectfileseditor.h
    genericprojectmanager.qrc
    genericprojectplugin.cpp genericprojectplugin.h
    genericprojectwizard.cpp genericprojectwizard.h
)

extend_qtc_plugin(GenericProjectManager
  CONDITION WITH_TESTS
  SOURCES genericprojectplugin_test.cpp
)
