import qbs 1.0

import QtcLibrary

Project {
    name: "FlameGraph"

    QtcDevHeaders { }

    QtcLibrary {
        Depends { name: "Qt"; submodules: ["qml", "quick", "gui"] }

        Group {
            name: "General"
            files: [
                "flamegraph.cpp", "flamegraph.h",
                "flamegraph_global.h",
                "flamegraphattached.h",
            ]
        }

        Group {
            name: "QML"
            prefix: "qml/"
            files: ["flamegraph.qrc"]
        }

        cpp.defines: base.concat("FLAMEGRAPH_LIBRARY")
    }
}
