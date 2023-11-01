if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
elseif (MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

add_qtc_plugin(LanguageClient
  PUBLIC_DEPENDS LanguageServerProtocol Qt::Core
  PLUGIN_DEPENDS ProjectExplorer Core TextEditor
  SOURCES
    callhierarchy.cpp callhierarchy.h
    client.cpp client.h
    clientrequest.cpp clientrequest.h
    currentdocumentsymbolsrequest.cpp currentdocumentsymbolsrequest.h
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
    languageclienttr.h
    locatorfilter.cpp locatorfilter.h
    lspinspector.cpp lspinspector.h
    progressmanager.cpp progressmanager.h
    semantichighlightsupport.cpp semantichighlightsupport.h
    snippet.cpp snippet.h
)
