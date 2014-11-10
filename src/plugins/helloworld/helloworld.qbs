import qbs 1.0

QtcPlugin {
    name: "HelloWorld"

    Depends { name: "Core" }
    Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "script"] }

    files: [
        "helloworldplugin.cpp",
        "helloworldplugin.h",
        "helloworldwindow.cpp",
        "helloworldwindow.h",
    ]
}

