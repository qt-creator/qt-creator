import qbs 1.0

QtcPlugin {
    name: "Terminal"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TerminalLib" }

    Group {
        name: "long description"
        files: "TerminalDescription.md"
        fileTags: "pluginjson.longDescription"
    }

    files: [
        "shellmodel.cpp",
        "shellmodel.h",
        "consolehost.cpp",
        "consolehost.h",
        "shellintegration.cpp",
        "shellintegration.h",
        "shortcutmap.cpp",
        "shortcutmap.h",
        "terminalconstants.h",
        "terminalicons.h",
        "terminalpane.cpp",
        "terminalpane.h",
        "terminalplugin.cpp",
        "terminalprocessimpl.cpp",
        "terminalprocessimpl.h",
        "terminalsettings.cpp",
        "terminalsettings.h",
        "terminaltr.h",
        "terminalwidget.cpp",
        "terminalwidget.h",
    ]

    Group {
        name: "images"
        fileTags: "qt.core.resource_data"
        files: [
            "images/keyboardlock.png",
            "images/keyboardlock@2x.png",
            "images/settingscategory_terminal.png",
            "images/settingscategory_terminal@2x.png",
            "images/terminal.png",
            "images/terminal@2x.png",
            "shellintegrations/shellintegration-bash.sh",
            "shellintegrations/shellintegration-env.zsh",
            "shellintegrations/shellintegration-login.zsh",
            "shellintegrations/shellintegration-profile.zsh",
            "shellintegrations/shellintegration-rc.zsh",
            "shellintegrations/shellintegration.fish",
            "shellintegrations/shellintegration.ps1",
            "shellintegrations/shellintegration-clink.lua",
        ]
    }
}
