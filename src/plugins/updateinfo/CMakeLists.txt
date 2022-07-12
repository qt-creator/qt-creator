add_qtc_plugin(UpdateInfo
  DEPENDS Qt5::Xml
  PLUGIN_DEPENDS Core
  PLUGIN_JSON_IN UPDATEINFO_EXPERIMENTAL_STR=true
  SOURCES
    settingspage.cpp
    settingspage.h
    updateinfoplugin.cpp
    updateinfoplugin.h
    updateinfotools.h
    updateinfotr.h
)
