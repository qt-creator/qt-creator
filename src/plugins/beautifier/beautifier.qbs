import qbs 1.0

QtcPlugin {
    name: "Beautifier"

    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.xml" }
    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    files: [
        "beautifierconstants.h",
        "beautifierplugin.cpp",
        "beautifiertool.h",
        "beautifiertool.cpp",
        "beautifiertr.h",
        "configurationdialog.cpp",
        "configurationdialog.h",
        "configurationeditor.cpp",
        "configurationeditor.h",
        "configurationpanel.cpp",
        "configurationpanel.h",
        "generalsettings.cpp",
        "generalsettings.h",
    ]

    Group {
        name: "ArtisticStyle"
        prefix: "artisticstyle/"
        files: [
            "artisticstyle.cpp",
            "artisticstyle.h",
        ]
    }

    Group {
        name: "ClangFormat"
        prefix: "clangformat/"
        files: [
            "clangformat.cpp",
            "clangformat.h",
        ]
    }

    Group {
        name: "Uncrustify"
        prefix: "uncrustify/"
        files: [
            "uncrustify.cpp",
            "uncrustify.h",
        ]
    }

    Group {
        name: "images"
        fileTags: "qt.core.resource_data"
        files: [
            "images/settingscategory_beautifier.png",
            "images/settingscategory_beautifier@2x.png",
        ]
    }
}
