import qbs 1.0

QtcPlugin {
    name: "ImageViewer"

    Depends { name: "Qt.svg"; required: false }
    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    Properties {
        condition: !Qt.svg.present
        cpp.defines: base.concat("QT_NO_SVG")
    }

    files: [
        "exportdialog.cpp",
        "exportdialog.h",
        "imageview.cpp",
        "imageview.h",
        "imageviewer.cpp",
        "imageviewer.h",
        "imageviewer.qrc",
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
