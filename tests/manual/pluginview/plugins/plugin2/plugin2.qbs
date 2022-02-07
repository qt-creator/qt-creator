QtcManualtest {
    name: "Manual test plugin2"
    targetName: "plugin2"
    type: [ "dynamiclibrary" ]

    Depends { name: "ExtensionSystem" }

    files: [
        "plugin2.cpp",
        "plugin2.h"
    ]
}
