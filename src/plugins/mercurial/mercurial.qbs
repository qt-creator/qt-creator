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

    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "authenticationdialog.cpp",
        "authenticationdialog.h",
        "authenticationdialog.ui",
        "clonewizard.cpp",
        "clonewizard.h",
        "clonewizardpage.cpp",
        "clonewizardpage.h",
        "commiteditor.cpp",
        "commiteditor.h",
        "constants.h",
        "mercurial.qrc",
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
