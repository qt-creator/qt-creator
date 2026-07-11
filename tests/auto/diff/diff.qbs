import qbs

Project {
    name: "Diff autotests"
    references: [
        "differ/differ.qbs",
        "inlinediff/inlinediff.qbs",
    ]
}
