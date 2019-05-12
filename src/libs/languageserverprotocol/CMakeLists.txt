add_qtc_library(LanguageServerProtocol
  DEPENDS Utils
  SOURCES
    basemessage.cpp basemessage.h
    client.cpp client.h
    clientcapabilities.cpp clientcapabilities.h
    completion.cpp completion.h
    diagnostics.cpp diagnostics.h
    icontent.h
    initializemessages.cpp initializemessages.h
    jsonkeys.h
    jsonobject.cpp jsonobject.h
    jsonrpcmessages.cpp jsonrpcmessages.h
    languagefeatures.cpp languagefeatures.h
    languageserverprotocol_global.h
    lsptypes.cpp lsptypes.h
    lsputils.cpp lsputils.h
    messages.cpp messages.h
    servercapabilities.cpp servercapabilities.h
    shutdownmessages.cpp shutdownmessages.h
    textsynchronization.cpp textsynchronization.h
    workspace.cpp workspace.h
)
