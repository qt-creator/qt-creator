find_package(Qt6 COMPONENTS Charts QUIET)

add_qtc_plugin(AppStatisticsMonitor
  CONDITION TARGET Qt6::Charts
  DEPENDS Qt6::Charts
  SKIP_TRANSLATION
  PLUGIN_DEPENDS Core ProjectExplorer
  SOURCES
    appstatisticsmonitorplugin.cpp
    chart.cpp chart.h
    manager.cpp manager.h
    idataprovider.h idataprovider.cpp
)

