import qbs 1.0

QtcPlugin {
    name: "Fossil"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "VcsBase" }

    files: [
        "constants.h",
        "fossilclient.cpp", "fossilclient.h",
        "fossilplugin.cpp", "fossilplugin.h",
        "fossilsettings.cpp", "fossilsettings.h",
        "commiteditor.cpp", "commiteditor.h",
        "fossilcommitwidget.cpp", "fossilcommitwidget.h",
        "fossileditor.cpp", "fossileditor.h",
        "annotationhighlighter.cpp", "annotationhighlighter.h",
        "pullorpushdialog.cpp", "pullorpushdialog.h", "pullorpushdialog.ui",
        "branchinfo.h",
        "configuredialog.cpp", "configuredialog.h", "configuredialog.ui",
        "revisioninfo.h",
        "fossil.qrc",
        "revertdialog.ui",
        "fossilcommitpanel.ui",
    ]

    Group {
        name: "Wizards"
        prefix: "wizard/"
        files: [
            "fossiljsextension.h", "fossiljsextension.cpp",
        ]
    }
}
