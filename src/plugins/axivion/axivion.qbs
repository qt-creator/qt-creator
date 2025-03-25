import qbs

QtcPlugin {
    name: "Axivion"

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["network", "sql", "widgets"] }

    files: [
        "axivionperspective.cpp",
        "axivionperspective.h",
        "axivionplugin.cpp",
        "axivionplugin.h",
        "axivionsettings.cpp",
        "axivionsettings.h",
        "axiviontr.h",
        "dynamiclistmodel.cpp",
        "dynamiclistmodel.h",
        "issueheaderview.cpp",
        "issueheaderview.h",
        "localbuild.cpp",
        "localbuild.h",
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

    Group {
        name: "long description"
        files: "AxivionDescription.md"
        fileTags: "pluginjson.longDescription"
    }

    Group {
        name: "images"
        files: "images/*.png"
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: "/axivion"
        Qt.core.resourceSourceBase: sourceDirectory
    }
}
