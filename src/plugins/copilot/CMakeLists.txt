add_qtc_plugin(Copilot
  PLUGIN_DEPENDS Core LanguageClient
  SOURCES
    authwidget.cpp authwidget.h
    copilot.qrc
    copilotplugin.cpp copilotplugin.h
    copilotclient.cpp copilotclient.h
    copilotsettings.cpp copilotsettings.h
    copilotoptionspage.cpp copilotoptionspage.h
    documentwatcher.cpp documentwatcher.h
    requests/getcompletions.h
    requests/checkstatus.h
    requests/signout.h
    requests/signininitiate.h
    requests/signinconfirm.h
)
