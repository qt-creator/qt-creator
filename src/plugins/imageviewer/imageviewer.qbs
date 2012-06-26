import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "ImageViewer"

    Depends { name: "Qt"; submodules: ["widgets", "svg"] }
    Depends { name: "Core" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        ".",
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "commandbutton.h",
        "imageviewerplugin.h",
        "imageviewerfactory.h",
        "imageviewerfile.h",
        "imageviewer.h",
        "imageview.h",
        "imageviewerconstants.h",
        "imagevieweractionhandler.h",
        "commandbutton.cpp",
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

