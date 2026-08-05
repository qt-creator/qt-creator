import qbs

Project {
    name: "TextEditor autotests"
    references: [
        "highlighter/highlighter.qbs",
        "mergeconflict/mergeconflict.qbs",
    ]
}
