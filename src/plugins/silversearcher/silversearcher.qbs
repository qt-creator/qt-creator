import qbs 1.0

QtcPlugin {
    name: "SilverSearcher"

    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "findinfilessilversearcher.cpp", "findinfilessilversearcher.h",
        "silversearcheroutputparser.cpp", "silversearcheroutputparser.h",
        "silversearcherplugin.cpp", "silversearcherplugin.h",
    ]

    QtcTestFiles {
        files: [
            "outputparser_test.cpp",
            "outputparser_test.h",
        ]
    }
}
