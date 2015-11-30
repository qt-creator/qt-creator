import qbs

QtcAutotest {
    name: "Generic highlighter specific rules autotest"
    Depends { name: "Qt.widgets" }
    Group {
        name: "Sources from TextEditor plugin"
        prefix: project.genericHighlighterDir + '/'
        files: [
            "context.cpp",
            "dynamicrule.cpp",
            "highlightdefinition.cpp",
            "itemdata.cpp",
            "keywordlist.cpp",
            "progressdata.cpp",
            "rule.cpp",
            "specificrules.cpp",
        ]
    }
    Group {
        name: "Test sources"
        files: "tst_specificrules.cpp"
    }

    cpp.includePaths: base.concat([project.genericHighlighterDir + "/../.."])
}
