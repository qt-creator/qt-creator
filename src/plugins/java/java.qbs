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
        "javakitaspect.cpp",
        "javakitaspect.h",
        "javalanguageserver.cpp",
        "javalanguageserver.h",
        "javaparser.cpp",
        "javaparser.h",
        "javaplugin.cpp",
        "javatr.h",
        "jdkmanager.cpp",
        "jdkmanager.h",
        "jdkoptionspage.cpp",
        "jdkoptionspage.h",
    ]

    QtcTestFiles {
        files: [
            "javaclasspath_test.cpp",
            "javaclasspath_test.h",
            "javakitaspect_test.cpp",
            "javakitaspect_test.h",
            "jdkmanager_test.cpp",
            "jdkmanager_test.h",
        ]
    }
}
