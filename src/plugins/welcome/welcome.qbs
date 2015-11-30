import qbs 1.0

QtcPlugin {
    name: "Welcome"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Qt.quick"; condition: product.condition; }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    files: [
        "welcomeplugin.cpp",
        "welcomeplugin.h",
        "welcome.qrc",
    ]
}
