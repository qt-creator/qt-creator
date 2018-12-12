import qbs 1.0

QtcPlugin {
    name: "Mercurial"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils"}

    Depends { name: "Core" }
    Depends { name: "DiffEditor" }
    Depends { name: "TextEditor" }
    Depends { name: "VcsBase" }

    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "authenticationdialog.cpp",
        "authenticationdialog.h",
        "authenticationdialog.ui",
        "commiteditor.cpp",
        "commiteditor.h",
        "constants.h",
        "mercurialclient.cpp",
        "mercurialclient.h",
        "mercurialcommitpanel.ui",
        "mercurialcommitwidget.cpp",
        "mercurialcommitwidget.h",
        "mercurialcontrol.cpp",
        "mercurialcontrol.h",
        "mercurialeditor.cpp",
        "mercurialeditor.h",
        "mercurialplugin.cpp",
        "mercurialplugin.h",
        "mercurialsettings.cpp",
        "mercurialsettings.h",
        "optionspage.cpp",
        "optionspage.h",
        "optionspage.ui",
        "revertdialog.cpp",
        "revertdialog.h",
        "revertdialog.ui",
        "srcdestdialog.cpp",
        "srcdestdialog.h",
        "srcdestdialog.ui",
    ]
}
