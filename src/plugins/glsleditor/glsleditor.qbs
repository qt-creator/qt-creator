import qbs 1.0

QtcPlugin {
    name: "GLSLEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "GLSL" }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "CppTools" }

    files: [
        "glslautocompleter.cpp",
        "glslautocompleter.h",
        "glslcompletionassist.cpp",
        "glslcompletionassist.h",
        "glsleditor.cpp",
        "glsleditor.h",
        "glsleditor.qrc",
        "glsleditorconstants.h",
        "glsleditorplugin.cpp",
        "glsleditorplugin.h",
        "glslhighlighter.cpp",
        "glslhighlighter.h",
        "glslindenter.cpp",
        "glslindenter.h",
    ]
}
