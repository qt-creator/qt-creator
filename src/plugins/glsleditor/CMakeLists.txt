add_qtc_plugin(GLSLEditor
  PLUGIN_CLASS GlslEditorPlugin
  DEPENDS GLSL
  PLUGIN_DEPENDS Core CppEditor TextEditor
  SOURCES
    glslautocompleter.cpp glslautocompleter.h
    glslcompletionassist.cpp glslcompletionassist.h
    glsleditor.cpp glsleditor.h
    glsleditorconstants.h
    glsleditorplugin.cpp
    glsleditortr.h
    glslhighlighter.cpp glslhighlighter.h
    glslindenter.cpp glslindenter.h
)

qtc_add_resources(GLSLEditor "images"
  PREFIX "/glsleditor"
  BASE "."
  FILES images/glslfile.png
)
