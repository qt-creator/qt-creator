import qbs

QtcPlugin {
    name: "Axivion"

    Depends { name: "Core" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "qtkeychain" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.network" }

    files: [
        "axivion.qrc",
        "axivionoutputpane.cpp",
        "axivionoutputpane.h",
        "axivionplugin.cpp",
        "axivionplugin.h",
        "axivionsettings.cpp",
        "axivionsettings.h",
        "axiviontr.h",
        "credentialquery.cpp",
        "credentialquery.h",
        "dynamiclistmodel.cpp",
        "dynamiclistmodel.h",
        "issueheaderview.cpp",
        "issueheaderview.h",
    ]

    cpp.includePaths: base.concat(["."]) // needed for the generated stuff below

    Group {
        name: "Dashboard Communication"
        prefix: "dashboard/"

        files: [
            "concat.cpp",
            "concat.h",
            "dto.cpp",
            "dto.h",
            "error.cpp",
            "error.h",
        ]
    }
}
