import qbs

Project {
    name: "Auto tests"

    references: [
        "bench/bench.qbs",
        "derived/derived.qbs",
        "dummy/dummy.qbs",
        "gui/gui.qbs",
        "quickauto/quickauto.qbs",
        "quickauto2/quickauto2.qbs",
        "quickauto3/quickauto3.qbs"
    ]
}
