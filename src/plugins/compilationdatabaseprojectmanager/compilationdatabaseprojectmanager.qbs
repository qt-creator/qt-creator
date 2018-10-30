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
        "compilationdatabaseutils.cpp",
        "compilationdatabaseutils.h",
        "compilationdatabaseprojectmanagerplugin.cpp",
        "compilationdatabaseprojectmanagerplugin.h",
    ]

    Group {
        name: "Tests"
        condition: qtc.testsEnabled
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
