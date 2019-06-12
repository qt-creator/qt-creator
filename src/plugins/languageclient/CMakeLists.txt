add_qtc_plugin(LanguageClient
  DEPENDS LanguageServerProtocol Qt5::Core
  PLUGIN_DEPENDS ProjectExplorer Core TextEditor
  SOURCES
    client.cpp client.h
    documentsymbolcache.cpp documentsymbolcache.h
    dynamiccapabilities.cpp dynamiccapabilities.h
    languageclient.qrc
    languageclientcompletionassist.cpp languageclientcompletionassist.h
    languageclientfunctionhint.cpp languageclientfunctionhint.h
    languageclienthoverhandler.cpp languageclienthoverhandler.h
    languageclientinterface.cpp languageclientinterface.h
    languageclientmanager.cpp languageclientmanager.h
    languageclientoutline.cpp languageclientoutline.h
    languageclientplugin.cpp languageclientplugin.h
    languageclientquickfix.cpp languageclientquickfix.h
    languageclientsettings.cpp languageclientsettings.h
    languageclientutils.cpp languageclientutils.h
    languageclient_global.h
    locatorfilter.cpp locatorfilter.h
    semantichighlightsupport.cpp semantichighlightsupport.h
)
