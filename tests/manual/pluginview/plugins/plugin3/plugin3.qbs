QtcManualtest {
    name: "Manual test plugin3"
    targetName: "plugin3"
    type: [ "dynamiclibrary" ]

    Depends { name: "ExtensionSystem" }
    Depends { name: "Manual test plugin2" }

    files: [
        "plugin3.cpp",
        "plugin3.h"
    ]
}
