add_qtc_plugin(GenericProjectManager
  PLUGIN_CLASS GenericProjectPlugin
  PLUGIN_DEPENDS Core ProjectExplorer QtSupport TextEditor
  PLUGIN_TEST_DEPENDS CppEditor
  PLUGIN_RECOMMENDS CppEditor
  SOURCES ${TEST_SOURCES}
    genericbuildconfiguration.cpp genericbuildconfiguration.h
    genericmakestep.cpp genericmakestep.h
    genericproject.cpp genericproject.h
    genericprojectconstants.h
    genericprojectfileseditor.cpp genericprojectfileseditor.h
    genericprojectmanagertr.h
    genericprojectplugin.cpp
    genericprojectwizard.cpp genericprojectwizard.h
)
