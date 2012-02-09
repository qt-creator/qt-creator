import qbs.base 1.0

DynamicLibrary {
    name: "aggregation"
    destination: "lib"

    cpp.includePaths: [
        ".",
        ".."
    ]
    cpp.defines: [
        "AGGREGATION_LIBRARY"
    ]

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }

    files: [
        "aggregation_global.h",
        "aggregate.h",
        "aggregate.cpp"
    ]
}
