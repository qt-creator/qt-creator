import qbs

StaticLibrary {
    name: "qtcjson"
    Depends { name: "cpp" }
    cpp.cxxLanguageVersion: "c++17"
    cpp.minimumMacosVersion: project.minimumMacosVersion
    files: [
        "json.cpp",
        "json.h",
    ]
    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
