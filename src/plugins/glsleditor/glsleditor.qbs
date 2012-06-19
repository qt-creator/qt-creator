import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "GLSLEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "CppTools" }
    Depends { name: "GLSL" }
    Depends { name: "CPlusPlus" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "../..",
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "glsleditor.h",
        "glsleditor_global.h",
        "glsleditoractionhandler.h",
        "glsleditorconstants.h",
        "glsleditoreditable.h",
        "glsleditorfactory.h",
        "glsleditorplugin.h",
        "glslfilewizard.h",
        "glslhighlighter.h",
        "glslautocompleter.h",
        "glslindenter.h",
        "glslhoverhandler.h",
        "glslcompletionassist.h",
        "reuse.h",
        "glsleditor.cpp",
        "glsleditoractionhandler.cpp",
        "glsleditoreditable.cpp",
        "glsleditorfactory.cpp",
        "glsleditorplugin.cpp",
        "glslfilewizard.cpp",
        "glslhighlighter.cpp",
        "glslautocompleter.cpp",
        "glslindenter.cpp",
        "glslhoverhandler.cpp",
        "glslcompletionassist.cpp",
        "reuse.cpp",
        "glsleditor.qrc",
        "GLSLEditor.mimetypes.xml"
    ]
}

