import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "Aggregation"

    cpp.includePaths: [
        ".",
        ".."
    ]

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }

    files: [
        "aggregation_global.h",
        "aggregate.h",
        "aggregate.cpp"
    ]
}
