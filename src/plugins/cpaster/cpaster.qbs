import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "CodePaster"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    cpp.includePaths: base.concat("../../shared/cpaster")

    files: [
        "codepasterprotocol.cpp",
        "codepasterprotocol.h",
        "codepastersettings.cpp",
        "codepastersettings.h",
        "columnindicatortextedit.cpp",
        "columnindicatortextedit.h",
        "cpasterconstants.h",
        "cpasterplugin.cpp",
        "cpasterplugin.h",
        "fileshareprotocol.cpp",
        "fileshareprotocol.h",
        "fileshareprotocolsettingspage.cpp",
        "fileshareprotocolsettingspage.h",
        "fileshareprotocolsettingswidget.ui",
        "kdepasteprotocol.cpp",
        "kdepasteprotocol.h",
        "pastebindotcaprotocol.cpp",
        "pastebindotcaprotocol.h",
        "pastebindotcomprotocol.cpp",
        "pastebindotcomprotocol.h",
        "pastebindotcomsettings.ui",
        "pasteselect.ui",
        "pasteselectdialog.cpp",
        "pasteselectdialog.h",
        "pasteview.cpp",
        "pasteview.h",
        "pasteview.ui",
        "protocol.cpp",
        "protocol.h",
        "settings.cpp",
        "settings.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
        "urlopenprotocol.cpp",
        "urlopenprotocol.h",
    ]

    Group {
        name: "Shared"
        prefix: "../../shared/cpaster/"
        files: [
            "cgi.cpp",
            "cgi.h",
            "splitter.cpp",
            "splitter.h",
        ]
    }
}
