QtcPlugin {
    name: "AppStatisticsMonitor"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt.charts"; required: false }

    condition: Qt.charts.present

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
