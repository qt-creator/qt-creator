import qbs

QtcPlugin {
    name: "CompilationDatabaseProjectManager"

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    files: [
        "compilationdatabaseconstants.h",
        "compilationdatabaseproject.cpp",
        "compilationdatabaseproject.h",
        "compilationdatabaseprojectmanagerplugin.cpp",
        "compilationdatabaseprojectmanagerplugin.h",
    ]
}
