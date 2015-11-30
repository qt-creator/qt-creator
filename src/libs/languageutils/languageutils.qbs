import qbs 1.0

QtcLibrary {
    name: "LanguageUtils"

    cpp.defines: base.concat([
        "LANGUAGEUTILS_BUILD_DIR"
    ])
    cpp.optimization: "fast"

    Depends { name: "Qt.core" }

    files: [
        "componentversion.cpp",
        "componentversion.h",
        "fakemetaobject.cpp",
        "fakemetaobject.h",
        "languageutils_global.h",
    ]
}

