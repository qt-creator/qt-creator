import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Macros"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "Locator" }
    Depends { name: "Find" }
    Depends { name: "TextEditor" }
    Depends { name: "app_version_header" }

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
        "macrooptionswidget.cpp",
        "macrooptionswidget.h",
        "macrooptionswidget.ui",
        "macros.qrc",
        "macros_global.h",
        "macrosconstants.h",
        "macrosplugin.cpp",
        "macrosplugin.h",
        "macrotextfind.cpp",
        "macrotextfind.h",
        "savedialog.cpp",
        "savedialog.h",
        "savedialog.ui",
        "texteditormacrohandler.cpp",
        "texteditormacrohandler.h",
    ]
}
