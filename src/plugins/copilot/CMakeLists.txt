add_qtc_plugin(Copilot
  PLUGIN_DEPENDS Core LanguageClient ProjectExplorer
  LONG_DESCRIPTION_MD Description.md
  SOURCES
    authwidget.cpp authwidget.h
    copilotclient.cpp copilotclient.h
    copilotconstants.h
    copilotplugin.cpp
    copilotsettings.cpp copilotsettings.h
    copilottr.h
    copilotrequests.h
)

qtc_add_resources(Copilot "images"
  PREFIX "/copilot"
  FILES
    images/copilot.png
    images/copilot@2x.png
)
