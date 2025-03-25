add_qtc_plugin(Axivion
  PLUGIN_DEPENDS
    Core Debugger ProjectExplorer TextEditor
  DEPENDS Qt::Network Qt::Sql Qt::Widgets ExtensionSystem Utils
  LONG_DESCRIPTION_MD AxivionDescription.md
  SOURCES
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
    localbuild.cpp localbuild.h
)

file(GLOB_RECURSE images RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} images/*)
qtc_add_resources(Axivion "images"
  PREFIX "/axivion"
  BASE "."
  FILES ${images}
)
