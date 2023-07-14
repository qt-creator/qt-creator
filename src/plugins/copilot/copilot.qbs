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
        "copilothoverhandler.cpp",
        "copilothoverhandler.h",
        "copilotplugin.cpp",
        "copilotplugin.h",
        "copilotprojectpanel.cpp",
        "copilotprojectpanel.h",
        "copilotsettings.cpp",
        "copilotsettings.h",
        "copilotsuggestion.cpp",
        "copilotsuggestion.h",
        "requests/checkstatus.h",
        "requests/getcompletions.h",
        "requests/signinconfirm.h",
        "requests/signininitiate.h",
        "requests/signout.h",
    ]
}
