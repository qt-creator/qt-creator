import qbs

Project {
    name: "QML code model autotests"
    references: [
        "check/check.qbs",
        "dependencies/dependencies.qbs",
        "ecmascript7/ecmascript7.qbs",
        "importscheck/importscheck.qbs",
    ]
}
