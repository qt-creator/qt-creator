add_qtc_plugin(Copilot
  PLUGIN_DEPENDS Core LanguageClient
  SOURCES
    authwidget.cpp authwidget.h
    copilot.qrc
    copilotclient.cpp copilotclient.h
    copilotconstants.h
    copilothoverhandler.cpp copilothoverhandler.h
    copilotplugin.cpp copilotplugin.h
    copilotprojectpanel.cpp copilotprojectpanel.h
    copilotsettings.cpp copilotsettings.h
    copilotsuggestion.cpp copilotsuggestion.h
    requests/checkstatus.h
    requests/getcompletions.h
    requests/signinconfirm.h
    requests/signininitiate.h
    requests/signout.h
)
