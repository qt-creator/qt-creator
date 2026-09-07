add_qtc_plugin(Copilot
  PLUGIN_DEPENDS Core LanguageClient ProjectExplorer
  LONG_DESCRIPTION_MD Description.md
  SOURCES
    authwidget.cpp authwidget.h
    copilot.qrc
    copilotclient.cpp copilotclient.h
    copilotconstants.h
    copilotplugin.cpp
    copilotsettings.cpp copilotsettings.h
    copilottr.h
    copilotrequests.h
)
