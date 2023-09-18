QtcTool {
    name: "sdktool"

    Depends { name: "Qt.core" }
    Depends { name: "app_version_header" }
    Depends { name: "sdktoolLib" }

    cpp.defines: base.concat([
        "UTILS_LIBRARY",
        qbs.targetOS.contains("macos")
            ? 'DATA_PATH="."'
            : qbs.targetOS.contains("windows") ? 'DATA_PATH="../share/qtcreator"'
                                               : 'DATA_PATH="../../share/qtcreator"'
    ])
    cpp.dynamicLibraries: {
        if (qbs.targetOS.contains("windows"))
            return ["user32", "shell32"]
    }
    Properties {
        condition: qbs.targetOS.contains("macos")
        cpp.frameworks: ["Foundation"]
    }

    files: [
        "main.cpp",
    ]
}
