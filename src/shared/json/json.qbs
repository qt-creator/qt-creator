import qbs

StaticLibrary {
    name: "qtcjson"
    Depends { name: "cpp" }
    cpp.cxxLanguageVersion: "c++11"
    files: [
        "json.cpp",
        "json.h",
    ]
    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [product.sourceDirectory]
    }
}
