Project {
    name: "Manual plugin test"

    QtcManualTest {
        name: "Manual plugin view test"

        Depends { name: "ExtensionSystem" }
        Depends { name: "Utils" }

        files: [
            "plugindialog.cpp",
            "plugindialog.h"
        ]
    }

    references: [
        "plugins/plugin1/plugin1.qbs",
        "plugins/plugin2/plugin2.qbs",
        "plugins/plugin3/plugin3.qbs",
    ]
}
