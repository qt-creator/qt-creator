import qbs 1.0

QtcPlugin {
    name: "SilverSearcher"

    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "findinfilessilversearcher.cpp", "findinfilessilversearcher.h",
        "silversearcherparser.cpp", "silversearcherparser.h",
        "silversearcherplugin.cpp",
        "silversearchertr.h",
    ]

    QtcTestFiles {
        files: [
            "silversearcherparser_test.cpp",
            "silversearcherparser_test.h",
        ]
    }
}
