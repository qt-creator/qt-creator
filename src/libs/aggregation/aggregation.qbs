QtcLibrary {
    name: "Aggregation"
    Depends { name: "Qt.core" }
    cpp.defines: base.concat("AGGREGATION_LIBRARY")

    files: [
        "aggregate.cpp",
        "aggregate.h",
        "aggregation_global.h",
    ]
}

