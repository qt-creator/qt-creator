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

    Group {
        name: "Tests"
        condition: qtc.testsEnabled
        files: [
            "outputparser_test.cpp",
            "outputparser_test.h",
        ]
    }
}
