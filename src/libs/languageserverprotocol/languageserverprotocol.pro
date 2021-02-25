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
    progresssupport.h \
    semantictokens.h \
    servercapabilities.h \
    shutdownmessages.h \
    textsynchronization.h \
    workspace.h \

SOURCES += \
    basemessage.cpp \
    client.cpp \
    clientcapabilities.cpp \
    completion.cpp \
    diagnostics.cpp \
    initializemessages.cpp \
    jsonobject.cpp \
    jsonrpcmessages.cpp \
    languagefeatures.cpp \
    lsptypes.cpp \
    lsputils.cpp \
    messages.cpp \
    progresssupport.cpp \
    semantictokens.cpp \
    servercapabilities.cpp \
    shutdownmessages.cpp \
    textsynchronization.cpp \
    workspace.cpp \
