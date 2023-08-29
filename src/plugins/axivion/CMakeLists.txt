add_qtc_plugin(Axivion
  PLUGIN_DEPENDS
    Core ProjectExplorer TextEditor
  DEPENDS Qt::Network Qt::Widgets ExtensionSystem Utils
  SOURCES
    axivion.qrc
    axivionoutputpane.cpp axivionoutputpane.h
    axivionplugin.cpp axivionplugin.h
    axivionprojectsettings.h axivionprojectsettings.cpp
    axivionquery.h axivionquery.cpp
    axivionresultparser.h axivionresultparser.cpp
    axivionsettings.cpp axivionsettings.h
    axiviontr.h
    dashboard/dto.cpp dashboard/dto.h
    dashboard/concat.cpp dashboard/concat.h
    dashboard/dashboardclient.cpp dashboard/dashboardclient.h
)
