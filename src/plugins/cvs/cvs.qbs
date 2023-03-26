import qbs 1.0

QtcPlugin {
    name: "CVS"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "VcsBase" }

    files: [
        "cvseditor.cpp",
        "cvseditor.h",
        "cvsplugin.cpp",
        "cvsplugin.h",
        "cvssettings.cpp",
        "cvssettings.h",
        "cvssubmiteditor.cpp",
        "cvssubmiteditor.h",
        "cvstr.h",
        "cvsutils.cpp",
        "cvsutils.h",
    ]
}
