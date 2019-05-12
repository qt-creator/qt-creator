add_qtc_plugin(CompilationDatabaseProjectManager
  DEPENDS Utils
  PLUGIN_DEPENDS Core CppTools ProjectExplorer TextEditor
  SOURCES
    compilationdatabaseconstants.h
    compilationdatabaseproject.cpp compilationdatabaseproject.h
    compilationdatabaseprojectmanagerplugin.cpp compilationdatabaseprojectmanagerplugin.h
    compilationdatabasetests.qrc
    compilationdatabaseutils.cpp compilationdatabaseutils.h
)

if (WITH_TESTS)
  target_sources(CompilationDatabaseProjectManager PRIVATE
    compilationdatabasetests.cpp compilationdatabasetests.h
  )
endif()
