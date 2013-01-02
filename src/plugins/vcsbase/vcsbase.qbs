import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "VcsBase"

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Find" }
    Depends { name: "cpp" }
    Depends { name: "Qt.widgets" }
    Depends { name: "CppTools" }
    Depends { name: "CPlusPlus" }

    files: [
        "VcsBase.mimetypes.xml",
        "baseannotationhighlighter.cpp",
        "baseannotationhighlighter.h",
        "basecheckoutwizard.cpp",
        "basecheckoutwizard.h",
        "basecheckoutwizardpage.cpp",
        "basecheckoutwizardpage.h",
        "basecheckoutwizardpage.ui",
        "basevcseditorfactory.cpp",
        "basevcseditorfactory.h",
        "basevcssubmiteditorfactory.cpp",
        "basevcssubmiteditorfactory.h",
        "checkoutjobs.cpp",
        "checkoutjobs.h",
        "checkoutprogresswizardpage.cpp",
        "checkoutprogresswizardpage.h",
        "checkoutprogresswizardpage.ui",
        "checkoutwizarddialog.cpp",
        "checkoutwizarddialog.h",
        "cleandialog.cpp",
        "cleandialog.h",
        "cleandialog.ui",
        "command.cpp",
        "command.h",
        "commonsettingspage.cpp",
        "commonsettingspage.h",
        "commonsettingspage.ui",
        "commonvcssettings.cpp",
        "commonvcssettings.h",
        "corelistener.cpp",
        "corelistener.h",
        "diffhighlighter.cpp",
        "diffhighlighter.h",
        "nicknamedialog.cpp",
        "nicknamedialog.h",
        "nicknamedialog.ui",
        "submiteditorfile.cpp",
        "submiteditorfile.h",
        "submiteditorwidget.cpp",
        "submiteditorwidget.h",
        "submiteditorwidget.ui",
        "submitfieldwidget.cpp",
        "submitfieldwidget.h",
        "submitfilemodel.cpp",
        "submitfilemodel.h",
        "vcsbase.qrc",
        "vcsbase_global.h",
        "vcsbaseclient.cpp",
        "vcsbaseclient.h",
        "vcsbaseclientsettings.cpp",
        "vcsbaseclientsettings.h",
        "vcsbaseconstants.h",
        "vcsbaseeditor.cpp",
        "vcsbaseeditor.h",
        "vcsbaseeditorparameterwidget.cpp",
        "vcsbaseeditorparameterwidget.h",
        "vcsbaseoptionspage.cpp",
        "vcsbaseoptionspage.h",
        "vcsbaseoutputwindow.cpp",
        "vcsbaseoutputwindow.h",
        "vcsbaseplugin.cpp",
        "vcsbaseplugin.h",
        "vcsbasesubmiteditor.cpp",
        "vcsbasesubmiteditor.h",
        "vcsconfigurationpage.cpp",
        "vcsconfigurationpage.h",
        "vcsplugin.cpp",
        "vcsplugin.h",
        "images/diff.png",
        "images/removesubmitfield.png",
        "images/submit.png",
    ]
}
