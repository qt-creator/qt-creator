import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Git"

    Depends { name: "qt"; submodules: ['gui'] }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }
    Depends { name: "VcsBase" }
    Depends { name: "Locator" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        ".",
        "gitorious",
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "branchadddialog.ui",
        "branchdialog.ui",
        "git.qrc",
        "gitsubmitpanel.ui",
        "remoteadditiondialog.ui",
        "stashdialog.ui",
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "branchadddialog.cpp",
        "branchadddialog.h",
        "branchdialog.cpp",
        "branchdialog.h",
        "branchmodel.cpp",
        "branchmodel.h",
        "changeselectiondialog.cpp",
        "changeselectiondialog.h",
        "changeselectiondialog.ui",
        "clonewizard.cpp",
        "clonewizard.h",
        "clonewizardpage.cpp",
        "clonewizardpage.h",
        "commitdata.cpp",
        "commitdata.h",
        "gitclient.cpp",
        "gitclient.h",
        "gitconstants.h",
        "giteditor.cpp",
        "giteditor.h",
        "gitplugin.cpp",
        "gitplugin.h",
        "gitsettings.cpp",
        "gitsettings.h",
        "gitsubmiteditor.cpp",
        "gitsubmiteditor.h",
        "gitsubmiteditorwidget.cpp",
        "gitsubmiteditorwidget.h",
        "gitutils.cpp",
        "gitutils.h",
        "gitversioncontrol.cpp",
        "gitversioncontrol.h",
        "remotedialog.cpp",
        "remotedialog.h",
        "remotedialog.ui",
        "remotemodel.cpp",
        "remotemodel.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
        "stashdialog.cpp",
        "stashdialog.h"
    ]

    Group {
        files: [
        "gitorious/gitorioushostwidget.ui",
        "gitorious/gitoriousprojectwidget.ui",
        "gitorious/gitoriousrepositorywizardpage.ui",
        "gitorious/gitorious.cpp",
        "gitorious/gitorious.h",
        "gitorious/gitoriousclonewizard.cpp",
        "gitorious/gitoriousclonewizard.h",
        "gitorious/gitorioushostwidget.cpp",
        "gitorious/gitorioushostwidget.h",
        "gitorious/gitorioushostwizardpage.cpp",
        "gitorious/gitorioushostwizardpage.h",
        "gitorious/gitoriousprojectwidget.cpp",
        "gitorious/gitoriousprojectwidget.h",
        "gitorious/gitoriousprojectwizardpage.cpp",
        "gitorious/gitoriousprojectwizardpage.h",
        "gitorious/gitoriousrepositorywizardpage.cpp",
        "gitorious/gitoriousrepositorywizardpage.h"
        ]
    }
}

