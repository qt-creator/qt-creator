add_qtc_plugin(UpdateInfo
  DEPENDS Qt::Xml
  PLUGIN_DEPENDS Core
  SOURCES
    updateinfoplugin.cpp
    updateinfoplugin.h
    updateinfoservice.h
    updateinfosettings.cpp
    updateinfosettings.h
    updateinfotools.h
    updateinfotr.h
)
