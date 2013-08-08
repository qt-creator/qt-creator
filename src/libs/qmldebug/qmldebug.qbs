import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "QmlDebug"

    cpp.defines: base.concat("QMLDEBUG_LIB")

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["gui", "network"] }

    files: [
        "baseenginedebugclient.cpp",
        "baseenginedebugclient.h",
        "basetoolsclient.cpp",
        "basetoolsclient.h",
        "declarativeenginedebugclient.cpp",
        "declarativeenginedebugclient.h",
        "declarativeenginedebugclientv2.h",
        "declarativetoolsclient.cpp",
        "declarativetoolsclient.h",
        "qdebugmessageclient.cpp",
        "qdebugmessageclient.h",
        "qmldebug_global.h",
        "qmldebugclient.cpp",
        "qmldebugclient.h",
        "qmldebugconstants.h",
        "qmlenginedebugclient.h",
        "qmloutputparser.cpp",
        "qmloutputparser.h",
        "qmlprofilereventlocation.h",
        "qmlprofilereventtypes.h",
        "qmlprofilertraceclient.cpp",
        "qmlprofilertraceclient.h",
        "qmltoolsclient.cpp",
        "qmltoolsclient.h",
        "qpacketprotocol.cpp",
        "qpacketprotocol.h",
        "qv8profilerclient.cpp",
        "qv8profilerclient.h",
    ]
}

