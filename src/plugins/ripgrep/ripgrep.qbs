import qbs 1.0

QtcPlugin {
    name: "Ripgrep"

    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "findinfilesripgrep.cpp", "findinfilesripgrep.h",
        "ripgrepparser.cpp", "ripgrepparser.h",
        "ripgrepplugin.cpp",
        "ripgreptr.h",
    ]

    QtcTestFiles {
        files: [
            "ripgrepengine_test.cpp",
            "ripgrepengine_test.h",
            "ripgrepparser_test.cpp",
            "ripgrepparser_test.h",
        ]
    }
}
