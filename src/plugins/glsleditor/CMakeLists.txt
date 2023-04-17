add_qtc_plugin(GLSLEditor
  PLUGIN_CLASS GlslEditorPlugin
  DEPENDS GLSL
  PLUGIN_DEPENDS Core CppEditor TextEditor
  SOURCES
    glslautocompleter.cpp glslautocompleter.h
    glslcompletionassist.cpp glslcompletionassist.h
    glsleditor.cpp glsleditor.h
    glsleditor.qrc
    glsleditorconstants.h
    glsleditorplugin.cpp glsleditorplugin.h
    glsleditortr.h
    glslhighlighter.cpp glslhighlighter.h
    glslindenter.cpp glslindenter.h
)
