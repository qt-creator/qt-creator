import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "GLSLEditor"

    Depends { name: "cpp" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "CppTools" }
    Depends { name: "GLSL" }
    Depends { name: "CPlusPlus" }
    cpp.defines: base.concat(["QT_NO_CAST_FROM_ASCII"])

    files: [
        "GLSLEditor.mimetypes.xml",
        "glslautocompleter.cpp",
        "glslautocompleter.h",
        "glslcompletionassist.cpp",
        "glslcompletionassist.h",
        "glsleditor.cpp",
        "glsleditor.h",
        "glsleditor.qrc",
        "glsleditor_global.h",
        "glsleditoractionhandler.cpp",
        "glsleditoractionhandler.h",
        "glsleditorconstants.h",
        "glsleditoreditable.cpp",
        "glsleditoreditable.h",
        "glsleditorfactory.cpp",
        "glsleditorfactory.h",
        "glsleditorplugin.cpp",
        "glsleditorplugin.h",
        "glslfilewizard.cpp",
        "glslfilewizard.h",
        "glslhighlighter.cpp",
        "glslhighlighter.h",
        "glslhoverhandler.cpp",
        "glslhoverhandler.h",
        "glslindenter.cpp",
        "glslindenter.h",
        "reuse.cpp",
        "reuse.h",
    ]
}
