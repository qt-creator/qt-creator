QtcPlugin {
    name: "ImageViewer"

    Depends { name: "Qt.svg"; required: false }
    Depends { name: "Qt.svgwidgets"; required: false }
    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    Properties {
        condition: !Qt.svg.present || !Qt.svgwidgets.present
        cpp.defines: base.concat("QT_NO_SVG")
    }

    files: [
        "exportdialog.cpp",
        "exportdialog.h",
        "multiexportdialog.cpp",
        "multiexportdialog.h",
        "imageview.cpp",
        "imageview.h",
        "imageviewer.cpp",
        "imageviewer.h",
        "imageviewerconstants.h",
        "imageviewerfile.cpp",
        "imageviewerfile.h",
        "imageviewerplugin.cpp",
        "imageviewerplugin.h",
        "imageviewertr.h",
    ]
}
