import qbs 1.0

QtcPlugin {
    name: "Macros"

    Depends { name: "Qt.widgets" }
    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "actionmacrohandler.cpp",
        "actionmacrohandler.h",
        "findmacrohandler.cpp",
        "findmacrohandler.h",
        "imacrohandler.cpp",
        "imacrohandler.h",
        "macro.cpp",
        "macro.h",
        "macroevent.cpp",
        "macroevent.h",
        "macrolocatorfilter.cpp",
        "macrolocatorfilter.h",
        "macromanager.cpp",
        "macromanager.h",
        "macrooptionspage.cpp",
        "macrooptionspage.h",
        "macros.qrc",
        "macrosconstants.h",
        "macrosplugin.cpp",
        "macrosplugin.h",
        "macrostr.h",
        "macrotextfind.cpp",
        "macrotextfind.h",
        "savedialog.cpp",
        "savedialog.h",
        "texteditormacrohandler.cpp",
        "texteditormacrohandler.h",
    ]
}
