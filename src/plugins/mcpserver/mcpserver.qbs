import qbs

QtcPlugin {
    name: "mcpserver"

    Depends { name: "app_version_header" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["network", "widgets"] }

    files: [
        "httpparser.cpp",
        "httpparser.h",
        "httpresponse.cpp",
        "httpresponse.h",
        "issuesmanager.cpp",
        "issuesmanager.h",
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
        prefix: "mcpserver/"
        files: [
            "mcpicon.png",
            "mcpicon@2x.png",
        ]
        fileTags: "qt.core.resource_data"
    }

    Group {
        name: "schemas"
        prefix: "mcpserver/"
        files: [
            "issues-schema.json",
        ]
        fileTags: "qt.core.resource_data"
    }
}
