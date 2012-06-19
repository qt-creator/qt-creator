import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Mercurial"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }
    Depends { name: "VcsBase" }
    Depends { name: "Locator" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "mercurialplugin.cpp",
        "optionspage.cpp",
        "mercurialcontrol.cpp",
        "mercurialclient.cpp",
        "annotationhighlighter.cpp",
        "mercurialeditor.cpp",
        "revertdialog.cpp",
        "srcdestdialog.cpp",
        "mercurialcommitwidget.cpp",
        "commiteditor.cpp",
        "clonewizardpage.cpp",
        "clonewizard.cpp",
        "mercurialsettings.cpp",
        "mercurialplugin.h",
        "constants.h",
        "optionspage.h",
        "mercurialcontrol.h",
        "mercurialclient.h",
        "annotationhighlighter.h",
        "mercurialeditor.h",
        "revertdialog.h",
        "srcdestdialog.h",
        "mercurialcommitwidget.h",
        "commiteditor.h",
        "clonewizardpage.h",
        "clonewizard.h",
        "mercurialsettings.h",
        "optionspage.ui",
        "revertdialog.ui",
        "srcdestdialog.ui",
        "mercurialcommitpanel.ui",
        "mercurial.qrc"
    ]
}

