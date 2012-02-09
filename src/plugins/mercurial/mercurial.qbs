import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Mercurial"

    Depends { name: "qt"; submodules: ['gui'] }
    Depends { name: "utils" }
    Depends { name: "extensionsystem" }
    Depends { name: "aggregation" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "find" }
    Depends { name: "VCSBase" }
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

