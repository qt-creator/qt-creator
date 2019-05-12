add_qtc_plugin(GLSLEditor
  DEPENDS GLSL
  PLUGIN_DEPENDS Core CppTools TextEditor
  SOURCES
    glslautocompleter.cpp glslautocompleter.h
    glslcompletionassist.cpp glslcompletionassist.h
    glsleditor.cpp glsleditor.h
    glsleditor.qrc
    glsleditorconstants.h
    glsleditorplugin.cpp glsleditorplugin.h
    glslhighlighter.cpp glslhighlighter.h
    glslindenter.cpp glslindenter.h
)
