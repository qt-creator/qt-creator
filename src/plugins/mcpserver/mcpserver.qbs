import qbs

QtcPlugin {
    name: "mcpserver"

    Depends { name: "app_version_header" }
    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "McpServerLib" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["network", "widgets"] }

    files: [
        "mcpcommands.cpp",
        "mcpcommands.h",
        "mcpserverplugin.cpp",
        "mcpserverconstants.h",
        "mcpserverinspector.cpp",
        "mcpserverinspector.h",
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

    Group {
        name: "schemas"
        prefix: "schemas/"
        files: [
            "search-results-schema.json",
        ]
        fileTags: "qt.core.resource_data"
    }
}
