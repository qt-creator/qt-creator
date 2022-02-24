import qbs

Project {
    name: "Tests"
    references: [
        "auto/auto.qbs",
        "manual/manual.qbs",
        "tools/qml-ast2dot/qml-ast2dot.qbs",
        "unit/unit.qbs",
    ]
}
