import qbs 1.0

QtcPlugin {
    name: "CVS"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "VcsBase" }

    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "cvs.qrc",
        "cvsclient.cpp",
        "cvsclient.h",
        "cvsconstants.h",
        "cvscontrol.cpp",
        "cvscontrol.h",
        "cvseditor.cpp",
        "cvseditor.h",
        "cvsplugin.cpp",
        "cvsplugin.h",
        "cvssettings.cpp",
        "cvssettings.h",
        "cvssubmiteditor.cpp",
        "cvssubmiteditor.h",
        "cvsutils.cpp",
        "cvsutils.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
    ]
}
