import qbs

QtcAutotest {
    name: "Highlighter engine autotest"
    Depends { name: "Core" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" }
    Group {
        name: "Sources from TextEditor plugin"
        prefix: project.genericHighlighterDir + '/'
        files: [
            "context.h", "context.cpp",
            "dynamicrule.h", "dynamicrule.cpp",
            "highlightdefinition.h", "highlightdefinition.cpp",
            "highlighter.h", "highlighter.cpp",
            "itemdata.h", "itemdata.cpp",
            "keywordlist.h", "keywordlist.cpp",
            "progressdata.h", "progressdata.cpp",
            "rule.h", "rule.cpp",
            "specificrules.h", "specificrules.cpp"
        ]
    }
    Group {
        name: "Test sources"
        files: [
            "formats.h", "formats.cpp",
            "highlightermock.h", "highlightermock.cpp",
            "tst_highlighterengine.cpp"
        ]
    }
    Group {
        name: "Drop-in sources for the plugin"
        files: [
            "textdocumentlayout.h", "textdocumentlayout.cpp",
            "syntaxhighlighter.h", "syntaxhighlighter.cpp",
            "tabsettings.h"
        ]
    }

    cpp.defines: base.concat(["TEXTEDITOR_LIBRARY"]) // For Windows
    cpp.includePaths: base.concat([
        path,
        project.genericHighlighterDir + "/../..",
    ])
}
