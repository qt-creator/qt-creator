QtcLibrary {
    name: "QmlDebug"

    cpp.defines: base.concat("QMLDEBUG_LIBRARY")

    Depends { name: "Qt"; submodules: ["gui", "network"] }
    Depends { name: "Utils" }
    Depends { name: "Tracing" }

    files: [
        "baseenginedebugclient.cpp",
        "baseenginedebugclient.h",
        "basetoolsclient.cpp",
        "basetoolsclient.h",
        "qdebugmessageclient.cpp",
        "qdebugmessageclient.h",
        "qmldebug_global.h",
        "qmldebugtr.h",
        "qmldebugclient.cpp",
        "qmldebugclient.h",
        "qmldebugconnection.cpp",
        "qmldebugconnection.h",
        "qmldebugconnectionmanager.cpp",
        "qmldebugconnectionmanager.h",
        "qmldebugconstants.h",
        "qmlenginecontrolclient.cpp",
        "qmlenginecontrolclient.h",
        "qmlenginedebugclient.h",
        "qmlevent.cpp",
        "qmlevent.h",
        "qmleventlocation.cpp",
        "qmleventlocation.h",
        "qmleventtype.cpp",
        "qmleventtype.h",
        "qmlprofilereventtypes.h",
        "qmlprofilertraceclient.cpp",
        "qmlprofilertraceclient.h",
        "qmltoolsclient.cpp",
        "qmltoolsclient.h",
        "qmltypedevent.cpp",
        "qmltypedevent.h",
        "qpacketprotocol.cpp",
        "qpacketprotocol.h",
    ]
}
