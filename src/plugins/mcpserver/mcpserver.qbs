import qbs

QtcPlugin {
    name: "mcpserver"

    Depends { name: "app_version_header" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["httpserver", "network", "widgets"] }

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
        "plugin.cpp",
        "pluginconstants.h",
        "plugintr.h",
    ]

    // TODO handle mcp_discovery.json.in
}
