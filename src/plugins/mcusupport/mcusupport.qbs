import qbs 1.0

QtcPlugin {
    name: "McuSupport"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "BareMetal" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Debugger" }
    Depends { name: "CMakeProjectManager" }
    Depends { name: "QtSupport" }

    files: [
        "mcuabstractpackage.h",
        "mcupackage.cpp",
        "mcupackage.h",
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
        "mcusupportsdk.cpp",
        "mcusupportsdk.h",
        "mcusupportrunconfiguration.cpp",
        "mcusupportrunconfiguration.h",
        "mcusupportversiondetection.cpp",
        "mcusupportversiondetection.h",
        "mcusupportcmakemapper.h",
        "mcusupportcmakemapper.cpp",
        "mcutargetdescription.h",
        "mcukitinformation.cpp",
        "mcukitinformation.h"
    ]
}
