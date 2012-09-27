import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "LanguageUtils"

    cpp.includePaths: base.concat("../3rdparty/cplusplus")
    cpp.defines: base.concat([
        "QT_CREATOR",
        "LANGUAGEUTILS_BUILD_DIR"
    ])
    cpp.optimization: "fast"

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }

    files: [
        "componentversion.cpp",
        "componentversion.h",
        "fakemetaobject.cpp",
        "fakemetaobject.h",
        "languageutils_global.h",
    ]
}

