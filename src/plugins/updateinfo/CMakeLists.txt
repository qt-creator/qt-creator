add_qtc_plugin(UpdateInfo
  DEPENDS Qt::Xml
  PLUGIN_DEPENDS Core
  SOURCES
    settingspage.cpp
    settingspage.h
    updateinfoplugin.cpp
    updateinfoplugin.h
    updateinfotools.h
    updateinfotr.h
)
