add_qtc_plugin(Java
  PLUGIN_DEPENDS Core LanguageClient ProjectExplorer TextEditor
  SOURCES
    java_global.h
    javaclasspath.cpp javaclasspath.h
    javaconstants.h
    javaeditor.cpp javaeditor.h
    javakitaspect.cpp javakitaspect.h
    javalanguageserver.cpp javalanguageserver.h
    javaplugin.cpp
    javatr.h
    jdkmanager.cpp jdkmanager.h
    jdkoptionspage.cpp jdkoptionspage.h
)

extend_qtc_plugin(Java
  CONDITION WITH_TESTS
  SOURCES
    javaclasspath_test.cpp javaclasspath_test.h
    javakitaspect_test.cpp javakitaspect_test.h
    jdkmanager_test.cpp jdkmanager_test.h
)
