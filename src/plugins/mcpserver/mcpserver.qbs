import qbs

QtcPlugin {
    name: "mcpserver"

    Depends { name: "app_version_header" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: [ "network", "widgets"] }
    Depends { name: "Qt.httpserver"; required: false }

    condition: Qt.httpserver.present

    files: [
        "issuesmanager.cpp",
        "issuesmanager.h",
        "mcp.qrc",
        "mcpserver.cpp",
        "mcpserver.h",
        "mcpservertest.cpp",
        "mcpservertest.h",
        "mcpcommands.cpp",
        "mcpcommands.h",
        "mcpserverplugin.cpp",
        "mcpserverconstants.h",
        "mcpservertr.h",
    ]

    Group {
        name: "images"
        prefix: "images/"
        files: [
            "mcpicon.png",
            "mcpicon@2x.png",
        ]
        fileTags: "qt.core.resource_data"
    }
}
