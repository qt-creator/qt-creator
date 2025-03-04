add_qtc_library(QmlDebug
  DEPENDS Qt::Network Utils
  SOURCES
    baseenginedebugclient.cpp baseenginedebugclient.h
    basetoolsclient.cpp basetoolsclient.h
    qdebugmessageclient.cpp qdebugmessageclient.h
    qmldebug_global.h
    qmldebugtr.h
    qmldebugclient.cpp qmldebugclient.h
    qmldebugconnection.cpp qmldebugconnection.h
    qmldebugconnectionmanager.cpp qmldebugconnectionmanager.h
    qmldebugconstants.h
    qmlenginecontrolclient.cpp qmlenginecontrolclient.h
    qmlenginedebugclient.h
    qmltoolsclient.cpp qmltoolsclient.h
    qpacketprotocol.cpp qpacketprotocol.h
)
