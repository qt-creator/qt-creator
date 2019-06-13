import qbs

Project {
    name: "Memcheck autotests"
    condition: !qbs.targetOS.contains("windows")
    references: [
        "testapps/testapps.qbs",
        "modeldemo.qbs"
    ]
}
