import qbs 1.0

import QtcPlugin

QtcPlugin {
    name: "GLSLEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Aggregation" }
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
        "glslhighlighterfactory.cpp",
        "glslhighlighterfactory.h",
        "glslhoverhandler.cpp",
        "glslhoverhandler.h",
        "glslindenter.cpp",
        "glslindenter.h",
        "reuse.cpp",
        "reuse.h",
    ]
}
