import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Git"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }
    Depends { name: "VcsBase" }
    Depends { name: "Locator" }

    Depends { name: "cpp" }
    cpp.defines: base.concat(["QT_NO_CAST_FROM_ASCII"])
    cpp.includePaths: [
        ".",
        "gitorious",
        "gerrit",
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
        "resetdialog.cpp",
        "resetdialog.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
        "stashdialog.cpp",
        "stashdialog.h"
    ]

    Group {
        prefix: "gitorious/"
        files: [
            "gitorioushostwidget.ui",
            "gitoriousprojectwidget.ui",
            "gitoriousrepositorywizardpage.ui",
            "gitorious.cpp",
            "gitorious.h",
            "gitoriousclonewizard.cpp",
            "gitoriousclonewizard.h",
            "gitorioushostwidget.cpp",
            "gitorioushostwidget.h",
            "gitorioushostwizardpage.cpp",
            "gitorioushostwizardpage.h",
            "gitoriousprojectwidget.cpp",
            "gitoriousprojectwidget.h",
            "gitoriousprojectwizardpage.cpp",
            "gitoriousprojectwizardpage.h",
            "gitoriousrepositorywizardpage.cpp",
            "gitoriousrepositorywizardpage.h"
        ]
    }

    Group {
        prefix: "gerrit/"
        files: [
            "gerritdialog.cpp",
            "gerritdialog.h",
            "gerritmodel.cpp",
            "gerritmodel.h",
            "gerritoptionspage.cpp",
            "gerritoptionspage.h",
            "gerritparameters.cpp",
            "gerritparameters.h",
            "gerritplugin.cpp",
            "gerritplugin.h"
        ]
    }
}

