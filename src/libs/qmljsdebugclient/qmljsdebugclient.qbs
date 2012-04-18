import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "QmlJSDebugClient"

    cpp.includePaths: [
        ".",
        ".."
    ]
    cpp.defines: [
        "QMLJSDEBUGCLIENT_LIBRARY",
        "QMLJSDEBUGCLIENT_LIB"
    ]

    Depends { name: "cpp" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.network" }
    Depends { name: "symbianutils" }

    files: [
        "qdeclarativedebugclient.cpp",
        "qdeclarativeoutputparser.cpp",
        "qdeclarativeoutputparser.h",
        "qmljsdebugclient_global.h",
        "qmljsdebugclientconstants.h",
        "qmlprofilereventlist.h",
        "qmlprofilertraceclient.cpp",
        "qpacketprotocol.cpp",
        "qv8profilerclient.cpp",
        "qv8profilerclient.h",
        "qdeclarativedebugclient.h",
        "qdeclarativeenginedebug.cpp",
        "qdeclarativeenginedebug.h",
        "qmlprofilereventtypes.h",
        "qmlprofilertraceclient.h",
        "qpacketprotocol.h",
        "qdebugmessageclient.cpp",
        "qmlprofilereventlist.cpp",
        "qdebugmessageclient.h"
    ]

    ProductModule {
        Depends { name: "cpp" }
        Depends { name: "symbianutils" }
        cpp.includePaths: [
            "."
        ]
    }
}

