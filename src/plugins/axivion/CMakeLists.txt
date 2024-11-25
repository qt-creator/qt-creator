add_qtc_plugin(Axivion
  PLUGIN_DEPENDS
    Core Debugger ProjectExplorer TextEditor
  DEPENDS Qt::Network Qt::Widgets ExtensionSystem Utils
  LONG_DESCRIPTION_MD AxivionDescription.md
  SOURCES
    axivion.qrc
    axivionperspective.cpp axivionperspective.h
    axivionplugin.cpp axivionplugin.h
    axivionsettings.cpp axivionsettings.h
    axiviontr.h
    dashboard/dto.cpp dashboard/dto.h
    dashboard/concat.cpp dashboard/concat.h
    dashboard/error.h dashboard/error.cpp
    dashboard/error.cpp dashboard/error.h
    dynamiclistmodel.cpp dynamiclistmodel.h
    issueheaderview.cpp issueheaderview.h
)
