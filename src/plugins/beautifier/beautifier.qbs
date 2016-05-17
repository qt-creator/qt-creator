import qbs 1.0

QtcPlugin {
    name: "Beautifier"

    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.xml" }
    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "DiffEditor" }
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
        "command.cpp",
        "command.h",
        "configurationdialog.cpp",
        "configurationdialog.h",
        "configurationdialog.ui",
        "configurationeditor.cpp",
        "configurationeditor.h",
        "configurationpanel.cpp",
        "configurationpanel.h",
        "configurationpanel.ui",
        "generaloptionspage.cpp",
        "generaloptionspage.h",
        "generaloptionspage.ui",
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
            "artisticstyleoptionspage.cpp",
            "artisticstyleoptionspage.h",
            "artisticstyleoptionspage.ui",
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
            "clangformatoptionspage.cpp",
            "clangformatoptionspage.h",
            "clangformatoptionspage.ui",
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
            "uncrustifyoptionspage.cpp",
            "uncrustifyoptionspage.h",
            "uncrustifyoptionspage.ui",
            "uncrustifysettings.cpp",
            "uncrustifysettings.h"
        ]
    }
}
