add_qtc_plugin(LanguageClient
  PUBLIC_DEPENDS LanguageServerProtocol Qt5::Core app_version
  PLUGIN_DEPENDS ProjectExplorer Core TextEditor
  SOURCES
    client.cpp client.h
    diagnosticmanager.cpp diagnosticmanager.h
    documentsymbolcache.cpp documentsymbolcache.h
    dynamiccapabilities.cpp dynamiccapabilities.h
    languageclient.qrc
    languageclientcompletionassist.cpp languageclientcompletionassist.h
    languageclientfunctionhint.cpp languageclientfunctionhint.h
    languageclientformatter.cpp languageclientformatter.h
    languageclienthoverhandler.cpp languageclienthoverhandler.h
    languageclientinterface.cpp languageclientinterface.h
    languageclientmanager.cpp languageclientmanager.h
    languageclientoutline.cpp languageclientoutline.h
    languageclientplugin.cpp languageclientplugin.h
    languageclientquickfix.cpp languageclientquickfix.h
    languageclientsettings.cpp languageclientsettings.h
    languageclientsymbolsupport.cpp languageclientsymbolsupport.h
    languageclientutils.cpp languageclientutils.h
    languageclient_global.h
    locatorfilter.cpp locatorfilter.h
    lspinspector.cpp lspinspector.h
    progressmanager.cpp progressmanager.h
    semantichighlightsupport.cpp semantichighlightsupport.h
    snippet.cpp snippet.h
)
