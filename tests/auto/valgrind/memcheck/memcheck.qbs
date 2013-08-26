import qbs

Project {
    name: "Memcheck autotests"
    references: [
        "testapps/testapps.qbs",
        "modeldemo.qbs",
        "parsertests.qbs",
        "testrunner.qbs"
    ]
}
