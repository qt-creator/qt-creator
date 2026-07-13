import qbs

QtcLibrary {
    name: "qtcjson"
    type: "staticlibrary"

    enforceInternalLinkage: false
    hasCMakeProjectFile: false
    useQt: false

    files: [
        "json.cpp",
        "json.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
