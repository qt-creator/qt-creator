import qbs 1.0

Project {
    name: "Aggregation"

    QtcLibrary {
        Depends { name: "Qt.core" }
        cpp.defines: base.concat("AGGREGATION_LIBRARY")

        files: [
            "aggregate.cpp",
            "aggregate.h",
            "aggregation_global.h",
        ]
    }
}

