import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Locator"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "script"] }
    Depends { name: "Core" }
    Depends { name: "cpp" }

    cpp.includePaths: [
        ".",
        "..",
        "generichighlighter",
        "tooltip",
        "snippets",
        "codeassist",
        "../../libs",
        buildDirectory
    ]

    files: [
        "directoryfilter.ui",
        "filesystemfilter.ui",
        "locator.qrc",
        "settingspage.ui",
        "basefilefilter.cpp",
        "basefilefilter.h",
        "commandlocator.cpp",
        "commandlocator.h",
        "directoryfilter.cpp",
        "directoryfilter.h",
        "executefilter.h",
        "filesystemfilter.cpp",
        "filesystemfilter.h",
        "ilocatorfilter.cpp",
        "locator_global.h",
        "locatorconstants.h",
        "locatorfiltersfilter.cpp",
        "locatorfiltersfilter.h",
        "locatormanager.cpp",
        "locatormanager.h",
        "locatorplugin.cpp",
        "locatorplugin.h",
        "opendocumentsfilter.cpp",
        "opendocumentsfilter.h",
        "settingspage.cpp",
        "settingspage.h",
        "executefilter.cpp",
        "ilocatorfilter.h",
        "locatorwidget.cpp",
        "locatorwidget.h",
        "images/locator.png",
        "images/reload.png"
    ]
}

