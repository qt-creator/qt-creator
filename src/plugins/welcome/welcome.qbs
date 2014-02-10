import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "Welcome"
    minimumQtVersion: "5.1"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Qt.quick"; condition: product.condition; }
    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    files: [
        "welcomeplugin.cpp",
        "welcomeplugin.h",
    ]
}
