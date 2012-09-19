import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "LanguageUtils"

    cpp.includePaths: [
        ".",
        "..",
        "../3rdparty/cplusplus"
    ]
    cpp.defines: base.concat([
        "QT_CREATOR",
        "LANGUAGEUTILS_BUILD_DIR"
    ])
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

