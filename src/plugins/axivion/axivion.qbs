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
        "axiviontr.h",
        "dashboard/dashboardclient.cpp",
        "dashboard/dashboardclient.h",
    ]

    cpp.includePaths: base.concat(["."]) // needed for the generated stuff below

    Group {
        name: "Generated DTOs"
        prefix: "dashboard/"

        files: [
            "concat.cpp",
            "concat.h",
            "dto.cpp",
            "dto.h",
        ]
    }
}
