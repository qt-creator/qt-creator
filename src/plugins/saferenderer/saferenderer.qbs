import qbs

QtcPlugin {
    name: "SafeRenderer"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "saferenderer.cpp",
        "saferenderer.h",
        "saferenderer.qrc",
    ]
}
