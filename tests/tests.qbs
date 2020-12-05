import qbs

Project {
    name: "Tests"
    references: [
        "auto/auto.qbs",
        "manual/manual.qbs",
        "unit/unit.qbs",
    ]
}
