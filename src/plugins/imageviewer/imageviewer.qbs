import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "ImageViewer"

    Depends { name: "qt"; submodules: ['gui', 'svg'] }
    Depends { name: "utils" }
    Depends { name: "extensionsystem" }
    Depends { name: "aggregation" }
    Depends { name: "Core" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "imageviewerplugin.h",
        "imageviewerfactory.h",
        "imageviewerfile.h",
        "imageviewer.h",
        "imageview.h",
        "imageviewerconstants.h",
        "imagevieweractionhandler.h",
        "imageviewerplugin.cpp",
        "imageviewerfactory.cpp",
        "imageviewerfile.cpp",
        "imageviewer.cpp",
        "imageview.cpp",
        "imagevieweractionhandler.cpp",
        "imageviewertoolbar.ui",
        "imageviewer.qrc",
        "ImageViewer.mimetypes.xml"
    ]
}




