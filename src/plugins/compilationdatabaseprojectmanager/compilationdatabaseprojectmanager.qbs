import qbs

QtcPlugin {
    name: "CompilationDatabaseProjectManager"

    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    files: [
        "compilationdatabaseconstants.h",
        "compilationdatabaseproject.cpp",
        "compilationdatabaseproject.h",
        "compilationdatabaseutils.cpp",
        "compilationdatabaseutils.h",
        "compilationdatabaseprojectmanagerplugin.cpp",
        "compilationdatabaseprojectmanagerplugin.h",
        "compilationdbparser.cpp",
        "compilationdbparser.h",
    ]

    QtcTestFiles {
        files: [
            "compilationdatabasetests.cpp",
            "compilationdatabasetests.h",
            "compilationdatabasetests.qrc",
        ]
    }

    Group {
        name: "Test resources"
        prefix: "database_samples/"
        fileTags: []
        files: ["**/*"]
    }
}
