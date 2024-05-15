QtcPlugin {
    name: "AppStatisticsMonitor"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "appstatisticsmonitorplugin.cpp",
        "appstatisticsmonitortr.h",
        "chart.h",
        "chart.cpp",
        "idataprovider.h",
        "idataprovider.cpp",
        "manager.h",
        "manager.cpp",
    ]
}
