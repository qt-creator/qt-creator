add_qtc_plugin(ZenMode
  PLUGIN_DEPENDS
    Core
    TextEditor
  DEPENDS
    Qt::Widgets
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    zenmodeplugin.cpp
    zenmodepluginconstants.h
    zenmodeplugintr.h
    zenmodesettings.cpp
    zenmodesettings.h
)

qtc_add_resources(ZenMode "images"
  PREFIX "/zenmode"
  BASE "."
  FILES
    images/settingscategory_zenmode.png
    images/settingscategory_zenmode@2x.png
    images/zenmode.png
    images/zenmode@2x.png
    images/distractionfree.png
    images/distractionfree@2x.png
)
