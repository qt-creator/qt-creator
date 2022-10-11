import qbs 1.0

QtcPlugin {
    name: "AppStatisticsMonitor"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }

    files: [
        "appstatisticsmonitorplugin.cpp",
        "chart.h",
        "chart.cpp",
        "manager.h",
        "manager.cpp",
        "idataprovider.h",
        "idataprovider.cpp",
        "tr.h"
    ]
}

