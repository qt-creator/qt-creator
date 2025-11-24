import qbs

QtcAutotest {
    name: "QmlEvent autotest"
    Depends { name: "QmlDebug" }
    files: ["tst_qmlevent.cpp", "tst_qmlevent.h"]
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
    Depends { name: "Tracing" }
    Depends { name: "Qt"; submodules: "network" }
    files: ["tst_qmlprofilertraceclient.cpp", "tst_qmlprofilertraceclient.h", "tests.qrc"]
}
