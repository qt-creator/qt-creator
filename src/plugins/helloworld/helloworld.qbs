import qbs 1.0

QtcPlugin {
    name: "HelloWorld"

    Depends { name: "Core" }
    Depends { name: "Qt"; submodules: ["widgets"] }

    files: [
        "helloworldplugin.cpp",
        "helloworldplugin.h",
        "helloworldwindow.cpp",
        "helloworldwindow.h",
        "helloworldtr.h"
    ]
}

