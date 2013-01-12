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

    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "branchadddialog.cpp",
        "branchadddialog.h",
        "branchadddialog.ui",
        "branchcheckoutdialog.cpp",
        "branchcheckoutdialog.h",
        "branchcheckoutdialog.ui",
        "branchdialog.cpp",
        "branchdialog.h",
        "branchdialog.ui",
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
        "git.qrc",
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
        "gitsubmitpanel.ui",
        "gitutils.cpp",
        "gitutils.h",
        "gitversioncontrol.cpp",
        "gitversioncontrol.h",
        "mergetool.cpp",
        "mergetool.h",
        "remoteadditiondialog.ui",
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
        "stashdialog.h",
        "stashdialog.ui",
    ]

    Group {
        prefix: "gitorious/"
        files: [
            "gitorious.cpp",
            "gitorious.h",
            "gitoriousclonewizard.cpp",
            "gitoriousclonewizard.h",
            "gitorioushostwidget.cpp",
            "gitorioushostwidget.h",
            "gitorioushostwidget.ui",
            "gitorioushostwizardpage.cpp",
            "gitorioushostwizardpage.h",
            "gitoriousprojectwidget.cpp",
            "gitoriousprojectwidget.h",
            "gitoriousprojectwidget.ui",
            "gitoriousprojectwizardpage.cpp",
            "gitoriousprojectwizardpage.h",
            "gitoriousrepositorywizardpage.cpp",
            "gitoriousrepositorywizardpage.h",
            "gitoriousrepositorywizardpage.ui",
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
            "gerritplugin.h",
        ]
    }
}
