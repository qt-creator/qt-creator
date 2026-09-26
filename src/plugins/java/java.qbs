import qbs 1.0

QtcPlugin {
    name: "Java"

    Depends { name: "Qt.widgets" }

    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "LanguageClient" }
    Depends { name: "LanguageServerProtocol" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    files: [
        "java_global.h",
        "javaclasspath.cpp",
        "javaclasspath.h",
        "javaconstants.h",
        "javaeditor.cpp",
        "javaeditor.h",
        "javalanguageserver.cpp",
        "javalanguageserver.h",
        "javaplugin.cpp",
        "javatr.h",
    ]

    QtcTestFiles {
        files: [
            "javaclasspath_test.cpp",
            "javaclasspath_test.h",
        ]
    }
}
