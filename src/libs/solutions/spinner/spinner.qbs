QtcLibrary {
    name: "Spinner"

    Depends { name: "Qt.widgets" }

    cpp.defines: base.concat("SPINNER_LIBRARY")

    files: [
        "spinner.cpp",
        "spinner.h",
        "spinner_global.h",
    ]

    Group {
        name: "images"
        prefix: "icons/"
        Qt.core.resourceSourceBase: sourceDirectory
        files: [
            "spinner_large.png",
            "spinner_large@2x.png",
            "spinner_medium.png",
            "spinner_medium@2x.png",
            "spinner_small.png",
            "spinner_small@2x.png",
        ]
        fileTags: "qt.core.resource_data"
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.sourceDirectory + "/.."
    }
}

