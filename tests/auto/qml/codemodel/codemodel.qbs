import qbs

Project {
    name: "QML code model autotests"
    references: ["check/check.qbs", "importscheck/importscheck.qbs",
                 "dependencies/dependencies.qbs"]
}
