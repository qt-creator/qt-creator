add_qtc_plugin(CompilationDatabaseProjectManager
  DEPENDS Utils
  PLUGIN_DEPENDS Core CppEditor ProjectExplorer TextEditor
  SOURCES
    compilationdatabaseconstants.h
    compilationdatabaseproject.cpp
    compilationdatabaseproject.h
    compilationdatabaseprojectmanagerplugin.cpp
    compilationdatabaseprojectmanagertr.h
    compilationdatabaseutils.cpp
    compilationdatabaseutils.h
    compilationdbparser.cpp
    compilationdbparser.h
)

extend_qtc_plugin(CompilationDatabaseProjectManager
  CONDITION WITH_TESTS
  SOURCES
    compilationdatabasetests.cpp
    compilationdatabasetests.h
)

file(GLOB_RECURSE test_resources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} database_samples/*)
qtc_add_resources(CompilationDatabaseProjectManager "test_resouces"
  CONDITION WITH_TESTS
  PREFIX "/"
  BASE "."
  FILES ${test_resources}
)
