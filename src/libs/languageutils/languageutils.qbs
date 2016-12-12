import qbs 1.0

Project {
    name: "LanguageUtils"

    QtcDevHeaders { }

    QtcLibrary {
        cpp.defines: base.concat(["LANGUAGEUTILS_LIBRARY"])
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
}
