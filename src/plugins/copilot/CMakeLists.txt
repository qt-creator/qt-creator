add_qtc_plugin(Copilot
  PLUGIN_DEPENDS Core LanguageClient
  LONG_DESCRIPTION_MD Description.md
  SOURCES
    authwidget.cpp authwidget.h
    copilot.qrc
    copilotclient.cpp copilotclient.h
    copilotconstants.h
    copilotplugin.cpp
    copilotprojectpanel.cpp copilotprojectpanel.h
    copilotsettings.cpp copilotsettings.h
    requests/checkstatus.h
    requests/getcompletions.h
    requests/signinconfirm.h
    requests/signininitiate.h
    requests/signout.h
)
