import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "Welcome"
    minimumQtVersion: "5.1"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Qt.quick"; condition: product.condition; }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "welcomeplugin.cpp",
        "welcomeplugin.h",
    ]
}
