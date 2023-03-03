import qbs 1.0

QtcPlugin {
    name: "Copilot"

    Depends { name: "Core" }
    Depends { name: "LanguageClient" }
    Depends { name: "TextEditor" }
    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }

    files: [
        "authwidget.cpp",
        "authwidget.h",
        "copilot.qrc",
        "copilotplugin.cpp",
        "copilotplugin.h",
        "copilotclient.cpp",
        "copilotclient.h",
        "copilotsettings.cpp",
        "copilotsettings.h",
        "copilotoptionspage.cpp",
        "copilotoptionspage.h",
        "requests/getcompletions.h",
        "requests/checkstatus.h",
        "requests/signout.h",
        "requests/signininitiate.h",
        "requests/signinconfirm.h",
    ]
}
