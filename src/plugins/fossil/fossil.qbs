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
        "annotationhighlighter.cpp", "annotationhighlighter.h",
        "branchinfo.h",
        "commiteditor.cpp", "commiteditor.h",
        "configuredialog.cpp", "configuredialog.h",
        "constants.h",
        "fossil.qrc",
        "fossilclient.cpp", "fossilclient.h",
        "fossilcommitwidget.cpp", "fossilcommitwidget.h",
        "fossileditor.cpp", "fossileditor.h",
        "fossilplugin.cpp", "fossilplugin.h",
        "fossilsettings.cpp", "fossilsettings.h",
        "fossiltr.h",
        "pullorpushdialog.cpp", "pullorpushdialog.h",
        "revisioninfo.h",
    ]

    Group {
        name: "Wizards"
        prefix: "wizard/"
        files: [
            "fossiljsextension.h", "fossiljsextension.cpp",
        ]
    }
}
