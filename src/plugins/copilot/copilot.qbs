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
        "copilot.qrc",
        "copilotclient.cpp",
        "copilotclient.h",
        "copilotconstants.h",
        "copilotplugin.cpp",
        "copilotsettings.cpp",
        "copilotsettings.h",
        "copilottr.h",
        "requests/checkstatus.h",
        "requests/getcompletions.h",
        "requests/signinconfirm.h",
        "requests/signininitiate.h",
        "requests/signout.h",
    ]

    Group {
        name: "long description"
        files: "Description.md"
        fileTags: "pluginjson.longDescription"
    }
}
