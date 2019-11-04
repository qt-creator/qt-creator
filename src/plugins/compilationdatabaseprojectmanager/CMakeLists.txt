add_qtc_plugin(CompilationDatabaseProjectManager
  DEPENDS Utils
  PLUGIN_DEPENDS Core CppTools ProjectExplorer TextEditor
  SOURCES
    compilationdatabaseconstants.h
    compilationdatabaseproject.cpp compilationdatabaseproject.h
    compilationdatabaseprojectmanagerplugin.cpp compilationdatabaseprojectmanagerplugin.h
    compilationdatabaseutils.cpp compilationdatabaseutils.h
    compilationdbparser.cpp compilationdbparser.h
)

extend_qtc_plugin(CompilationDatabaseProjectManager
  CONDITION WITH_TESTS
  SOURCES
    compilationdatabasetests.cpp compilationdatabasetests.h compilationdatabasetests.qrc
)
