add_qtc_plugin(Axivion
  PLUGIN_DEPENDS
    Core ProjectExplorer TextEditor
  DEPENDS Qt::Network Qt::Widgets ExtensionSystem Utils qtkeychain
  SOURCES
    axivion.qrc
    axivionoutputpane.cpp axivionoutputpane.h
    axivionplugin.cpp axivionplugin.h
    axivionsettings.cpp axivionsettings.h
    axiviontr.h
    credentialquery.h credentialquery.cpp
    dashboard/dto.cpp dashboard/dto.h
    dashboard/concat.cpp dashboard/concat.h
    dashboard/error.h dashboard/error.cpp
    dashboard/error.cpp dashboard/error.h
    dynamiclistmodel.cpp dynamiclistmodel.h
    issueheaderview.cpp issueheaderview.h
)
