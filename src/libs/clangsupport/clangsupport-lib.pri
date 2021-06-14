shared {
    DEFINES += CLANGSUPPORT_BUILD_LIB
} else {
    DEFINES += CLANGSUPPORT_BUILD_STATIC_LIB
}

QT += network

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/clangsupportdebugutils.cpp \
    $$PWD/clangcodemodelclientinterface.cpp \
    $$PWD/clangcodemodelclientproxy.cpp \
    $$PWD/clangcodemodelconnectionclient.cpp \
    $$PWD/clangcodemodelserverinterface.cpp \
    $$PWD/clangcodemodelserverproxy.cpp \
    $$PWD/alivemessage.cpp \
    $$PWD/completionsmessage.cpp \
    $$PWD/requestcompletionsmessage.cpp \
    $$PWD/echomessage.cpp \
    $$PWD/endmessage.cpp \
    $$PWD/documentsopenedmessage.cpp \
    $$PWD/documentsclosedmessage.cpp \
    $$PWD/codecompletionchunk.cpp \
    $$PWD/codecompletion.cpp \
    $$PWD/connectionclient.cpp \
    $$PWD/connectionserver.cpp \
    $$PWD/diagnosticcontainer.cpp \
    $$PWD/annotationsmessage.cpp \
    $$PWD/filecontainer.cpp \
    $$PWD/fixitcontainer.cpp \
    $$PWD/followsymbolmessage.cpp \
    $$PWD/lineprefixer.cpp \
    $$PWD/messageenvelop.cpp \
    $$PWD/readmessageblock.cpp \
    $$PWD/referencesmessage.cpp \
    $$PWD/unsavedfilesupdatedmessage.cpp \
    $$PWD/requestannotationsmessage.cpp \
    $$PWD/requestfollowsymbolmessage.cpp \
    $$PWD/requestreferencesmessage.cpp \
    $$PWD/requesttooltipmessage.cpp \
    $$PWD/sourcelocationcontainer.cpp \
    $$PWD/sourcelocationscontainer.cpp \
    $$PWD/sourcerangecontainer.cpp \
    $$PWD/processcreator.cpp \
    $$PWD/processexception.cpp \
    $$PWD/processstartedevent.cpp \
    $$PWD/tokeninfocontainer.cpp \
    $$PWD/tooltipmessage.cpp \
    $$PWD/tooltipinfo.cpp \
    $$PWD/unsavedfilesremovedmessage.cpp \
    $$PWD/documentschangedmessage.cpp \
    $$PWD/documentvisibilitychangedmessage.cpp \
    $$PWD/writemessageblock.cpp \
    $$PWD/baseserverproxy.cpp

HEADERS += \
    $$PWD/clangsupportdebugutils.h \
    $$PWD/clangsupport_global.h \
    $$PWD/clangcodemodelclientinterface.h \
    $$PWD/clangcodemodelclientmessages.h \
    $$PWD/clangcodemodelclientproxy.h \
    $$PWD/clangcodemodelconnectionclient.h \
    $$PWD/clangcodemodelserverinterface.h \
    $$PWD/clangcodemodelservermessages.h \
    $$PWD/clangcodemodelserverproxy.h \
    $$PWD/alivemessage.h \
    $$PWD/completionsmessage.h \
    $$PWD/requestcompletionsmessage.h \
    $$PWD/echomessage.h \
    $$PWD/endmessage.h \
    $$PWD/documentsopenedmessage.h \
    $$PWD/documentsclosedmessage.h \
    $$PWD/codecompletionchunk.h \
    $$PWD/codecompletion.h \
    $$PWD/connectionclient.h \
    $$PWD/connectionserver.h \
    $$PWD/diagnosticcontainer.h \
    $$PWD/annotationsmessage.h \
    $$PWD/filecontainer.h \
    $$PWD/fixitcontainer.h \
    $$PWD/followsymbolmessage.h \
    $$PWD/ipcclientinterface.h \
    $$PWD/ipcinterface.h \
    $$PWD/ipcserverinterface.h \
    $$PWD/lineprefixer.h \
    $$PWD/messageenvelop.h \
    $$PWD/readmessageblock.h \
    $$PWD/referencesmessage.h \
    $$PWD/unsavedfilesupdatedmessage.h \
    $$PWD/requestannotationsmessage.h \
    $$PWD/requestfollowsymbolmessage.h \
    $$PWD/requestreferencesmessage.h \
    $$PWD/requesttooltipmessage.h \
    $$PWD/sourcelocationcontainer.h \
    $$PWD/sourcelocationscontainer.h \
    $$PWD/sourcerangecontainer.h \
    $$PWD/processcreator.h \
    $$PWD/processexception.h \
    $$PWD/processhandle.h \
    $$PWD/processstartedevent.h \
    $$PWD/tokeninfocontainer.h \
    $$PWD/tooltipmessage.h \
    $$PWD/tooltipinfo.h \
    $$PWD/unsavedfilesremovedmessage.h \
    $$PWD/documentschangedmessage.h \
    $$PWD/documentvisibilitychangedmessage.h \
    $$PWD/writemessageblock.h \
    $$PWD/ipcclientprovider.h \
    $$PWD/baseserverproxy.h

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
