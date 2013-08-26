import qbs

Project {
    name: "Generic highlighter autotests"
    property path genericHighlighterDir: project.ide_source_tree
            + "/src/plugins/texteditor/generichighlighter"
    references: [
        "highlighterengine/highlighterengine.qbs",
        "specificrules/specificrules.qbs"
    ]
}
