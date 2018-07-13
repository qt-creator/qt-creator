include(../../qtcreatorlibrary.pri)

DEFINES += LANGUAGESERVERPROTOCOL_LIBRARY

HEADERS += \
    basemessage.h \
    client.h \
    clientcapabilities.h \
    completion.h \
    diagnostics.h \
    icontent.h \
    initializemessages.h \
    jsonobject.h \
    jsonrpcmessages.h \
    languagefeatures.h \
    languageserverprotocol_global.h \
    lsptypes.h \
    lsputils.h \
    messages.h \
    servercapabilities.h \
    shutdownmessages.h \
    textsynchronization.h \
    workspace.h

SOURCES += \
    basemessage.cpp \
    clientcapabilities.cpp \
    completion.cpp \
    initializemessages.cpp \
    jsonobject.cpp \
    jsonrpcmessages.cpp \
    languagefeatures.cpp \
    lsptypes.cpp \
    lsputils.cpp \
    messages.cpp \
    servercapabilities.cpp \
    textsynchronization.cpp \
    workspace.cpp \
    client.cpp \
    shutdownmessages.cpp \
    diagnostics.cpp
