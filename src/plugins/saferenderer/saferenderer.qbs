import qbs

QtcPlugin {
    name: "SafeRenderer"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "saferenderer.h",
    ]

    Group {
        name: "wizards"
        fileTags: "qt.core.resource_data"
        files: [
            "wizards/**/*",
        ]
    }
}
