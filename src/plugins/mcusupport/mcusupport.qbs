import qbs 1.0

QtcPlugin {
    name: "McuSupport"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Debugger" }
    Depends { name: "CMakeProjectManager" }

    files: [
        "mcusupport.qrc",
        "mcusupport_global.h",
        "mcusupportconstants.h",
        "mcusupportdevice.cpp",
        "mcusupportdevice.h",
        "mcusupportoptions.cpp",
        "mcusupportoptions.h",
        "mcusupportoptionspage.cpp",
        "mcusupportoptionspage.h",
        "mcusupportplugin.cpp",
        "mcusupportplugin.h",
        "mcusupportrunconfiguration.cpp",
        "mcusupportrunconfiguration.h",
    ]
}
