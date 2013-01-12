import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Bookmarks"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }
    Depends { name: "Locator" }

    files: [
        "bookmark.cpp",
        "bookmark.h",
        "bookmarkmanager.cpp",
        "bookmarkmanager.h",
        "bookmarks.qrc",
        "bookmarks_global.h",
        "bookmarksplugin.cpp",
        "bookmarksplugin.h",
    ]
}

