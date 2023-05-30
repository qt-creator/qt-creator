import qbs

QtcPlugin {
    name: "Axivion"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.network" }

    files: [
        "axivion.qrc",
        "axivionoutputpane.cpp",
        "axivionoutputpane.h",
        "axivionplugin.cpp",
        "axivionplugin.h",
        "axivionprojectsettings.h",
        "axivionprojectsettings.cpp",
        "axivionquery.h",
        "axivionquery.cpp",
        "axivionresultparser.h",
        "axivionresultparser.cpp",
        "axivionsettings.cpp",
        "axivionsettings.h",
        "axivionsettingspage.cpp",
        "axivionsettingspage.h",
        "axiviontr.h",
    ]
}
