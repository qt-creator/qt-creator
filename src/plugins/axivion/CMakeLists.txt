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
    axivionsettingspage.cpp axivionsettingspage.h
    axiviontr.h
)
