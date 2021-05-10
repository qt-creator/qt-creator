import qbs

Project {
    name: "Widgets manualtests"

    condition: project.withAutotests

    references: [
        "crumblepath/crumblepath.qbs",
        "infolabel/infolabel.qbs",
        "manhattanstyle/manhattanstyle.qbs",
        "tracing/tracing.qbs",
    ]
}
