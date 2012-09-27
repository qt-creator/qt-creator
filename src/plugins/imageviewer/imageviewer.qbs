import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "ImageViewer"

    Depends { name: "Qt"; submodules: ["widgets", "svg"] }
    Depends { name: "Core" }

    files: [
        "ImageViewer.mimetypes.xml",
        "imageview.cpp",
        "imageview.h",
        "imageviewer.cpp",
        "imageviewer.h",
        "imageviewer.qrc",
        "imagevieweractionhandler.cpp",
        "imagevieweractionhandler.h",
        "imageviewerconstants.h",
        "imageviewerfactory.cpp",
        "imageviewerfactory.h",
        "imageviewerfile.cpp",
        "imageviewerfile.h",
        "imageviewerplugin.cpp",
        "imageviewerplugin.h",
        "imageviewertoolbar.ui",
    ]
}
