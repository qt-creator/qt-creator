contains(CONFIG, dll) {
    DEFINES += CODEMODELBACKENDIPC_BUILD_LIB
} else {
    DEFINES += CODEMODELBACKENDIPC_BUILD_STATIC_LIB
}

QT += network

DEFINES += CODEMODELBACKENDIPC_LIBRARY

INCLUDEPATH += $$PWD

SOURCES += $$PWD/ipcserverinterface.cpp \
    $$PWD/ipcserverproxy.cpp \
    $$PWD/ipcserver.cpp \
    $$PWD/ipcclientinterface.cpp \
    $$PWD/cmbendcommand.cpp \
    $$PWD/cmbalivecommand.cpp \
    $$PWD/ipcclientproxy.cpp \
    $$PWD/cmbcommands.cpp \
    $$PWD/writecommandblock.cpp \
    $$PWD/readcommandblock.cpp \
    $$PWD/ipcinterface.cpp \
    $$PWD/connectionserver.cpp \
    $$PWD/connectionclient.cpp \
    $$PWD/cmbechocommand.cpp \
    $$PWD/ipcclientdispatcher.cpp \
    $$PWD/cmbregistertranslationunitsforcodecompletioncommand.cpp \
    $$PWD/filecontainer.cpp \
    $$PWD/cmbunregistertranslationunitsforcodecompletioncommand.cpp \
    $$PWD/cmbcompletecodecommand.cpp \
    $$PWD/cmbcodecompletedcommand.cpp \
    $$PWD/codecompletion.cpp \
    $$PWD/codemodelbackendipc_global.cpp \
    $$PWD/cmbregisterprojectsforcodecompletioncommand.cpp \
    $$PWD/cmbunregisterprojectsforcodecompletioncommand.cpp \
    $$PWD/translationunitdoesnotexistcommand.cpp \
    $$PWD/codecompletionchunk.cpp \
    $$PWD/projectpartcontainer.cpp \
    $$PWD/projectpartsdonotexistcommand.cpp


HEADERS += $$PWD/codemodelbackendipc_global.h \
    $$PWD/ipcserverinterface.h \
    $$PWD/ipcserverproxy.h \
    $$PWD/ipcserver.h \
    $$PWD/ipcclientinterface.h \
    $$PWD/cmbendcommand.h \
    $$PWD/cmbalivecommand.h \
    $$PWD/ipcclientproxy.h \
    $$PWD/cmbcommands.h \
    $$PWD/writecommandblock.h \
    $$PWD/readcommandblock.h \
    $$PWD/ipcinterface.h \
    $$PWD/connectionserver.h \
    $$PWD/connectionclient.h \
    $$PWD/cmbechocommand.h \
    $$PWD/ipcclientdispatcher.h \
    $$PWD/cmbregistertranslationunitsforcodecompletioncommand.h \
    $$PWD/filecontainer.h \
    $$PWD/cmbunregistertranslationunitsforcodecompletioncommand.h \
    $$PWD/cmbcompletecodecommand.h \
    $$PWD/cmbcodecompletedcommand.h \
    $$PWD/codecompletion.h \
    $$PWD/cmbregisterprojectsforcodecompletioncommand.h \
    $$PWD/cmbunregisterprojectsforcodecompletioncommand.h \
    $$PWD/translationunitdoesnotexistcommand.h \
    $$PWD/codecompletionchunk.h \
    $$PWD/projectpartcontainer.h \
    $$PWD/projectpartsdonotexistcommand.h

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
