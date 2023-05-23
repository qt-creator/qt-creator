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
        "abstractsettings.h",
        "abstractsettings.cpp",
        "beautifier.qrc",
        "beautifierabstracttool.h",
        "beautifierconstants.h",
        "beautifierplugin.cpp",
        "beautifierplugin.h",
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
            "artisticstyleconstants.h",
            "artisticstylesettings.cpp",
            "artisticstylesettings.h"
        ]
    }

    Group {
        name: "ClangFormat"
        prefix: "clangformat/"
        files: [
            "clangformat.cpp",
            "clangformat.h",
            "clangformatconstants.h",
            "clangformatsettings.cpp",
            "clangformatsettings.h"
        ]
    }

    Group {
        name: "Uncrustify"
        prefix: "uncrustify/"
        files: [
            "uncrustify.cpp",
            "uncrustify.h",
            "uncrustifyconstants.h",
            "uncrustifysettings.cpp",
            "uncrustifysettings.h"
        ]
    }
}
