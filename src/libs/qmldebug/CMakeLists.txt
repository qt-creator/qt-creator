add_qtc_library(QmlDebug
  DEPENDS Qt5::Network Utils
  SOURCES
    baseenginedebugclient.cpp baseenginedebugclient.h
    basetoolsclient.cpp basetoolsclient.h
    qdebugmessageclient.cpp qdebugmessageclient.h
    qmldebug_global.h
    qmldebugclient.cpp qmldebugclient.h
    qmldebugcommandlinearguments.h
    qmldebugconnection.cpp qmldebugconnection.h
    qmldebugconnectionmanager.cpp qmldebugconnectionmanager.h
    qmldebugconstants.h
    qmlenginecontrolclient.cpp qmlenginecontrolclient.h
    qmlenginedebugclient.h
    qmloutputparser.cpp qmloutputparser.h
    qmltoolsclient.cpp qmltoolsclient.h
    qpacketprotocol.cpp qpacketprotocol.h
)
