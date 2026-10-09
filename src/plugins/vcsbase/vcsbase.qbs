import qbs 1.0

QtcPlugin {
    name: "VcsBase"

    Depends { name: "Qt.widgets" }
    Depends { name: "CPlusPlus" }
    Depends { name: "Spinner" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "DiffEditor" }

    pluginRecommends: [
        "CodePaster",
        "CppEditor"
    ]

    files: [
        "baseannotationhighlighter.cpp",
        "baseannotationhighlighter.h",
        "cleandialog.cpp",
        "cleandialog.h",
        "commonvcssettings.cpp",
        "commonvcssettings.h",
        "diffandloghighlighter.cpp",
        "diffandloghighlighter.h",
        "nicknamedialog.cpp",
        "nicknamedialog.h",
        "submiteditorfile.cpp",
        "submiteditorfile.h",
        "submiteditorwidget.cpp",
        "submiteditorwidget.h",
        "submitfieldwidget.cpp",
        "submitfieldwidget.h",
        "submitfilemodel.cpp",
        "submitfilemodel.h",
        "vcsbase_global.h", "vcsbasetr.h",
        "vcsbaseclient.cpp",
        "vcsbaseclient.h",
        "vcsbaseclientsettings.cpp",
        "vcsbaseclientsettings.h",
        "vcsbaseconstants.h",
        "vcsbasediffeditorcontroller.cpp",
        "vcsbasediffeditorcontroller.h",
        "vcsbaseeditor.cpp",
        "vcsbaseeditor.h",
        "vcsbaseeditorconfig.cpp",
        "vcsbaseeditorconfig.h",
        "vcsbaseplugin.cpp",
        "vcsbaseplugin.h",
        "vcsbasesubmiteditor.cpp",
        "vcsbasesubmiteditor.h",
        "vcschangesview.cpp",
        "vcschangesview.h",
        "vcscommand.cpp",
        "vcscommand.h",
        "vcsenums.h",
        "vcsfilestatus.h",
        "vcsoutputformatter.cpp",
        "vcsoutputformatter.h",
        "vcsoutputwindow.cpp",
        "vcsoutputwindow.h",
        "vcsplugin.cpp",
        "vcsplugin.h",
        "wizard/vcsconfigurationpage.cpp",
        "wizard/vcsconfigurationpage.h",
        "wizard/vcscommandpage.cpp",
        "wizard/vcscommandpage.h",
        "wizard/vcsjsextension.cpp",
        "wizard/vcsjsextension.h",
    ]

    Group {
        name: "images"
        fileTags: "qt.core.resource_data"
        files: [
            "images/diff_arrows.png",
            "images/diff_arrows@2x.png",
            "images/diff_documents.png",
            "images/diff_documents@2x.png",
            "images/settingscategory_vcs.png",
            "images/settingscategory_vcs@2x.png",
            "images/submit_arrow.png",
            "images/submit_arrow@2x.png",
            "images/submit_db.png",
            "images/submit_db@2x.png",
        ]
    }

    cpp.defines: base.concat(qtc.withPluginTests ? ['SRC_DIR="' + project.ide_source_tree + '"'] : [])
}
