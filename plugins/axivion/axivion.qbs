import qbs

QtcCommercialPlugin {
    name: "Axivion"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
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
        "axivionsettings.cpp",
        "axivionsettings.h",
        "axivionsettingspage.cpp",
        "axivionsettingspage.h",
        "axiviontr.h",
    ]
}
