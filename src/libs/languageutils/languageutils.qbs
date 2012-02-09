import qbs.base 1.0

DynamicLibrary {
    name: "LanguageUtils"
    destination: "lib"

    cpp.includePaths: [
        ".",
        "..",
        "../3rdparty/cplusplus"
    ]
    cpp.defines: [
        "QT_CREATOR",
        "LANGUAGEUTILS_BUILD_DIR"
    ]
    cpp.optimization: "fast"

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }

    files: [
        "languageutils_global.h",
        "fakemetaobject.h",
        "componentversion.h",
        "fakemetaobject.cpp",
        "componentversion.cpp"
    ]
}

