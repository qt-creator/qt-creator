contains(CONFIG, dll) {
    DEFINES += CLANGBACKENDIPC_BUILD_LIB
} else {
    DEFINES += CLANGBACKENDIPC_BUILD_STATIC_LIB
}

QT += network

DEFINES += CLANGBACKENDIPC_LIBRARY

INCLUDEPATH += $$PWD

SOURCES += $$PWD/ipcserverinterface.cpp \
    $$PWD/ipcserverproxy.cpp \
    $$PWD/ipcclientinterface.cpp \
    $$PWD/cmbendmessage.cpp \
    $$PWD/cmbalivemessage.cpp \
    $$PWD/ipcclientproxy.cpp \
    $$PWD/cmbmessages.cpp \
    $$PWD/writemessageblock.cpp \
    $$PWD/readmessageblock.cpp \
    $$PWD/ipcinterface.cpp \
    $$PWD/connectionserver.cpp \
    $$PWD/connectionclient.cpp \
    $$PWD/cmbechomessage.cpp \
    $$PWD/ipcclientdispatcher.cpp \
    $$PWD/cmbregistertranslationunitsforcodecompletionmessage.cpp \
    $$PWD/filecontainer.cpp \
    $$PWD/cmbunregistertranslationunitsforcodecompletionmessage.cpp \
    $$PWD/cmbcompletecodemessage.cpp \
    $$PWD/cmbcodecompletedmessage.cpp \
    $$PWD/codecompletion.cpp \
    $$PWD/cmbregisterprojectsforcodecompletionmessage.cpp \
    $$PWD/cmbunregisterprojectsforcodecompletionmessage.cpp \
    $$PWD/translationunitdoesnotexistmessage.cpp \
    $$PWD/codecompletionchunk.cpp \
    $$PWD/projectpartcontainer.cpp \
    $$PWD/projectpartsdonotexistmessage.cpp \
    $$PWD/lineprefixer.cpp \
    $$PWD/clangbackendipcdebugutils.cpp \
    $$PWD/diagnosticschangedmessage.cpp \
    $$PWD/diagnosticcontainer.cpp \
    $$PWD/sourcerangecontainer.cpp \
    $$PWD/sourcelocationcontainer.cpp \
    $$PWD/fixitcontainer.cpp \
    $$PWD/requestdiagnosticsmessage.cpp

HEADERS += \
    $$PWD/ipcserverinterface.h \
    $$PWD/ipcserverproxy.h \
    $$PWD/ipcclientinterface.h \
    $$PWD/cmbendmessage.h \
    $$PWD/cmbalivemessage.h \
    $$PWD/ipcclientproxy.h \
    $$PWD/cmbmessages.h \
    $$PWD/writemessageblock.h \
    $$PWD/readmessageblock.h \
    $$PWD/ipcinterface.h \
    $$PWD/connectionserver.h \
    $$PWD/connectionclient.h \
    $$PWD/cmbechomessage.h \
    $$PWD/ipcclientdispatcher.h \
    $$PWD/cmbregistertranslationunitsforcodecompletionmessage.h \
    $$PWD/filecontainer.h \
    $$PWD/cmbunregistertranslationunitsforcodecompletionmessage.h \
    $$PWD/cmbcompletecodemessage.h \
    $$PWD/cmbcodecompletedmessage.h \
    $$PWD/codecompletion.h \
    $$PWD/cmbregisterprojectsforcodecompletionmessage.h \
    $$PWD/cmbunregisterprojectsforcodecompletionmessage.h \
    $$PWD/translationunitdoesnotexistmessage.h \
    $$PWD/codecompletionchunk.h \
    $$PWD/projectpartcontainer.h \
    $$PWD/projectpartsdonotexistmessage.h \
    $$PWD/container_common.h \
    $$PWD/clangbackendipc_global.h \
    $$PWD/lineprefixer.h \
    $$PWD/clangbackendipcdebugutils.h \
    $$PWD/diagnosticschangedmessage.h \
    $$PWD/diagnosticcontainer.h \
    $$PWD/sourcerangecontainer.h \
    $$PWD/sourcelocationcontainer.h \
    $$PWD/fixitcontainer.h \
    $$PWD/requestdiagnosticsmessage.h

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
