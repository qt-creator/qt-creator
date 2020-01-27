import qbs 1.0

QtcPlugin {
    name: "MarkdownViewer"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "markdownbrowser.cpp",
        "markdownbrowser.h",
        "markdowndocument.cpp",
        "markdowndocument.h",
        "markdowneditor.cpp",
        "markdowneditor.h",
        "markdownviewer.cpp",
        "markdownviewer.h",
        "markdownviewerconstants.h",
        "markdownviewerfactory.cpp",
        "markdownviewerfactory.h",
        "markdownviewerplugin.cpp",
        "markdownviewerplugin.h",
        "markdownviewerwidget.cpp",
        "markdownviewerwidget.h",
    ]
}
