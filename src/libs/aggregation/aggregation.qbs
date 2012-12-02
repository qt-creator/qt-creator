import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "Aggregation"

    cpp.defines: base.concat(["AGGREGATION_LIBRARY", "QT_NO_CAST_FROM_ASCII"])
    Depends { name: "cpp" }
    Depends { name: "Qt.core" }

    files: [
        "aggregate.cpp",
        "aggregate.h",
        "aggregation_global.h",
    ]
}
