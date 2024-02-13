import qbs.base 1.0

QtcPlugin {
    name: "EmacsKeys"

    Depends { name: "Qt.widgets" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "emacskeysconstants.h",
        "emacskeysplugin.cpp",
        "emacskeysstate.cpp",
        "emacskeysstate.h",
        "emacskeystr.h",
    ]
}
