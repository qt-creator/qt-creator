import qbs 1.0

QtcPlugin {
    name: "VcsBase"

    Depends { name: "Qt.widgets" }
    Depends { name: "Aggregation" }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "CppTools" }

    files: [
        "baseannotationhighlighter.cpp",
        "baseannotationhighlighter.h",
        "basecheckoutwizard.cpp",
        "basecheckoutwizard.h",
        "basecheckoutwizardfactory.cpp",
        "basecheckoutwizardfactory.h",
        "basecheckoutwizardpage.cpp",
        "basecheckoutwizardpage.h",
        "basecheckoutwizardpage.ui",
        "basevcseditorfactory.cpp",
        "basevcseditorfactory.h",
        "basevcssubmiteditorfactory.cpp",
        "basevcssubmiteditorfactory.h",
        "checkoutprogresswizardpage.cpp",
        "checkoutprogresswizardpage.h",
        "cleandialog.cpp",
        "cleandialog.h",
        "cleandialog.ui",
        "commonsettingspage.cpp",
        "commonsettingspage.h",
        "commonsettingspage.ui",
        "commonvcssettings.cpp",
        "commonvcssettings.h",
        "corelistener.cpp",
        "corelistener.h",
        "diffandloghighlighter.cpp",
        "diffandloghighlighter.h",
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
        "vcsbaseplugin.cpp",
        "vcsbaseplugin.h",
        "vcsbasesubmiteditor.cpp",
        "vcsbasesubmiteditor.h",
        "vcscommand.cpp",
        "vcscommand.h",
        "vcsoutputwindow.cpp",
        "vcsoutputwindow.h",
        "vcsplugin.cpp",
        "vcsplugin.h",
        "vcsprojectcache.cpp",
        "vcsprojectcache.h",
        "images/diff.png",
        "images/removesubmitfield.png",
        "images/submit.png",
        "wizard/vcsconfigurationpage.cpp",
        "wizard/vcsconfigurationpage.h",
        "wizard/vcsjsextension.cpp",
        "wizard/vcsjsextension.h",
    ]
}
