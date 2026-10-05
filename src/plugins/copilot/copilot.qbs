import qbs 1.0

QtcPlugin {
    name: "Copilot"

    Depends { name: "Core" }
    Depends { name: "LanguageClient" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }

    files: [
        "authwidget.cpp",
        "authwidget.h",
        "copilotclient.cpp",
        "copilotclient.h",
        "copilotconstants.h",
        "copilotplugin.cpp",
        "copilotsettings.cpp",
        "copilotsettings.h",
        "copilottr.h",
        "copilotrequests.h",
    ]

    Group {
        name: "long description"
        files: "Description.md"
        fileTags: "pluginjson.longDescription"
    }

    Group {
        name: "images"
        fileTags: "qt.core.resource_data"
        files: [
            "images/copilot.png",
            "images/copilot@2x.png",
        ]
    }
}
