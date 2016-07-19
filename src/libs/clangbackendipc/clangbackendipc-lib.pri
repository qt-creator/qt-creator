contains(CONFIG, dll) {
    DEFINES += CLANGBACKENDIPC_BUILD_LIB
} else {
    DEFINES += CLANGBACKENDIPC_BUILD_STATIC_LIB
}

QT += network

INCLUDEPATH += $$PWD

SOURCES += $$PWD/clangcodemodelserverinterface.cpp \
    $$PWD/clangcodemodelserverproxy.cpp \
    $$PWD/clangcodemodelclientinterface.cpp \
    $$PWD/cmbendmessage.cpp \
    $$PWD/cmbalivemessage.cpp \
    $$PWD/clangcodemodelclientproxy.cpp \
    $$PWD/writemessageblock.cpp \
    $$PWD/readmessageblock.cpp \
    $$PWD/ipcinterface.cpp \
    $$PWD/connectionserver.cpp \
    $$PWD/connectionclient.cpp \
    $$PWD/cmbechomessage.cpp \
    $$PWD/cmbregistertranslationunitsforeditormessage.cpp \
    $$PWD/filecontainer.cpp \
    $$PWD/cmbunregistertranslationunitsforeditormessage.cpp \
    $$PWD/cmbcompletecodemessage.cpp \
    $$PWD/cmbcodecompletedmessage.cpp \
    $$PWD/codecompletion.cpp \
    $$PWD/cmbregisterprojectsforeditormessage.cpp \
    $$PWD/cmbunregisterprojectsforeditormessage.cpp \
    $$PWD/translationunitdoesnotexistmessage.cpp \
    $$PWD/codecompletionchunk.cpp \
    $$PWD/projectpartcontainer.cpp \
    $$PWD/projectpartsdonotexistmessage.cpp \
    $$PWD/clangbackendipcdebugutils.cpp \
    $$PWD/diagnosticcontainer.cpp \
    $$PWD/sourcerangecontainer.cpp \
    $$PWD/sourcelocationcontainer.cpp \
    $$PWD/fixitcontainer.cpp \
    $$PWD/requestdocumentannotations.cpp \
    $$PWD/registerunsavedfilesforeditormessage.cpp \
    $$PWD/unregisterunsavedfilesforeditormessage.cpp \
    $$PWD/updatetranslationunitsforeditormessage.cpp \
    $$PWD/updatevisibletranslationunitsmessage.cpp \
    $$PWD/highlightingmarkcontainer.cpp \
    $$PWD/messageenvelop.cpp \
    $$PWD/ipcclientinterface.cpp \
    $$PWD/ipcserverinterface.cpp \
    $$PWD/clangcodemodelconnectionclient.cpp \
    $$PWD/documentannotationschangedmessage.cpp

HEADERS += \
    $$PWD/clangcodemodelserverinterface.h \
    $$PWD/clangcodemodelserverproxy.h \
    $$PWD/clangcodemodelclientinterface.h \
    $$PWD/cmbendmessage.h \
    $$PWD/cmbalivemessage.h \
    $$PWD/clangcodemodelclientproxy.h \
    $$PWD/writemessageblock.h \
    $$PWD/readmessageblock.h \
    $$PWD/ipcinterface.h \
    $$PWD/connectionserver.h \
    $$PWD/connectionclient.h \
    $$PWD/cmbechomessage.h \
    $$PWD/cmbregistertranslationunitsforeditormessage.h \
    $$PWD/filecontainer.h \
    $$PWD/cmbunregistertranslationunitsforeditormessage.h \
    $$PWD/cmbcompletecodemessage.h \
    $$PWD/cmbcodecompletedmessage.h \
    $$PWD/codecompletion.h \
    $$PWD/cmbregisterprojectsforeditormessage.h \
    $$PWD/cmbunregisterprojectsforeditormessage.h \
    $$PWD/translationunitdoesnotexistmessage.h \
    $$PWD/codecompletionchunk.h \
    $$PWD/projectpartcontainer.h \
    $$PWD/projectpartsdonotexistmessage.h \
    $$PWD/clangbackendipc_global.h \
    $$PWD/lineprefixer.h \
    $$PWD/clangbackendipcdebugutils.h \
    $$PWD/diagnosticcontainer.h \
    $$PWD/sourcerangecontainer.h \
    $$PWD/sourcelocationcontainer.h \
    $$PWD/fixitcontainer.h \
    $$PWD/requestdocumentannotations.h \
    $$PWD/registerunsavedfilesforeditormessage.h \
    $$PWD/unregisterunsavedfilesforeditormessage.h \
    $$PWD/updatetranslationunitsforeditormessage.h \
    $$PWD/updatevisibletranslationunitsmessage.h \
    $$PWD/highlightingmarkcontainer.h \
    $$PWD/messageenvelop.h \
    $$PWD/ipcclientinterface.h \
    $$PWD/ipcserverinterface.h \
    $$PWD/clangcodemodelconnectionclient.h \
    $$PWD/documentannotationschangedmessage.h

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
