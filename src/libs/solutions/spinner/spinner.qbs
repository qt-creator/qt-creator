QtcLibrary {
    name: "Spinner"

    Depends { name: "Qt.widgets" }

    cpp.defines: base.concat("SPINNER_LIBRARY")

    files: [
        "spinner.cpp",
        "spinner.h",
        "spinner.qrc",
        "spinner_global.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.sourceDirectory + "/.."
    }
}

