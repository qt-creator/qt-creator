import qbs

Project {
    QtcAutotest {
        name: "QmlEvent autotest"
        Depends { name: "QmlDebug" }
        files: ["tst_qmlevent.cpp", "tst_qmlevent.h"]
    }

    QtcAutotest {
        name: "BaseEngineDebugClient autotest"
        Depends { name: "QmlDebug" }
        Depends { name: "Qt"; submodules: "network" }
        files: ["tst_baseenginedebugclient.cpp"]
    }

    QtcAutotest {
        name: "QmlEventLocation autotest"
        Depends { name: "QmlDebug" }
        files: ["tst_qmleventlocation.cpp", "tst_qmleventlocation.h"]
    }

    QtcAutotest {
        name: "QmlEventType autotest"
        Depends { name: "QmlDebug" }
        files: ["tst_qmleventtype.cpp", "tst_qmleventtype.h"]
    }

    QtcAutotest {
        name: "QmlProfilerTraceClient autotest"
        Depends { name: "QmlDebug" }
        Depends { name: "Utils" }
        Depends { name: "Tracing"; required: false }
        Depends { name: "Qt"; submodules: "network" }
        condition: Tracing.present
        files: ["tst_qmlprofilertraceclient.cpp", "tst_qmlprofilertraceclient.h", "tests.qrc"]
    }
}
