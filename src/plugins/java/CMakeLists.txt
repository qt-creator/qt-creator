add_qtc_plugin(Java
  PLUGIN_DEPENDS Core LanguageClient ProjectExplorer TextEditor
  SOURCES
    java_global.h
    javaclasspath.cpp javaclasspath.h
    javaconstants.h
    javaeditor.cpp javaeditor.h
    javalanguageserver.cpp javalanguageserver.h
    javaplugin.cpp
    javatr.h
)

extend_qtc_plugin(Java
  CONDITION WITH_TESTS
  SOURCES
    javaclasspath_test.cpp javaclasspath_test.h
)
