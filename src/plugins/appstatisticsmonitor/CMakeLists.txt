add_qtc_plugin(AppStatisticsMonitor
  SKIP_TRANSLATION
  PLUGIN_DEPENDS Core ProjectExplorer
  SOURCES
    appstatisticsmonitorplugin.cpp
    chart.cpp chart.h
    manager.cpp manager.h
    idataprovider.h idataprovider.cpp
)

