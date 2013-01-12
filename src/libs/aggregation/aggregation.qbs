import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "Aggregation"

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    cpp.defines: base.concat("AGGREGATION_LIBRARY")

    files: [
        "aggregate.cpp",
        "aggregate.h",
        "aggregation_global.h",
    ]
}
