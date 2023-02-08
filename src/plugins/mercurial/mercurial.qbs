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
        "commiteditor.cpp",
        "commiteditor.h",
        "constants.h",
        "mercurialclient.cpp",
        "mercurialclient.h",
        "mercurialcommitwidget.cpp",
        "mercurialcommitwidget.h",
        "mercurialeditor.cpp",
        "mercurialeditor.h",
        "mercurialplugin.cpp",
        "mercurialplugin.h",
        "mercurialsettings.cpp",
        "mercurialsettings.h",
        "mercurialtr.h",
        "revertdialog.cpp",
        "revertdialog.h",
        "srcdestdialog.cpp",
        "srcdestdialog.h"
    ]
}
