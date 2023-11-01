QtcLibrary {
    name: "ptyqt"
    type: "staticlibrary"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.network"; condition: qbs.targetOS.contains("windows") }
    Depends { name: "winpty"; condition: qbs.targetOS.contains("windows") }

    files: [
        "iptyprocess.h",
        "ptyqt.cpp",
        "ptyqt.h",
    ]

    Group {
        name: "ptyqt UNIX files"
        condition: qbs.targetOS.contains("unix")
        files: [
            "unixptyprocess.cpp",
            "unixptyprocess.h",
        ]
    }

    Group {
        name: "ptyqt Windows files"
        condition: qbs.targetOS.contains("windows")
        files: [
            "conptyprocess.cpp",
            "conptyprocess.h",
            "winptyprocess.cpp",
            "winptyprocess.h",
        ]
    }

    Export {
        Depends { name: "cpp" }
        Depends { name: "winpty"; condition: qbs.targetOS.contains("windows") }
        cpp.includePaths: exportingProduct.sourceDirectory
    }
}
