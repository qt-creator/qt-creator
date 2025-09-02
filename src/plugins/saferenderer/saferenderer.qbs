import qbs

QtcPlugin {
    name: "SafeRenderer"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "saferenderer.h",
        "saferenderer.qrc",
    ]
}
