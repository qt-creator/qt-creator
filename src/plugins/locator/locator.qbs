import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Locator"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "script"] }
    Depends { name: "Core" }
    Depends { name: "cpp" }

    cpp.includePaths: base.concat([
        "generichighlighter",
        "tooltip",
        "snippets",
        "codeassist"
    ])

    files: [
        "basefilefilter.cpp",
        "basefilefilter.h",
        "commandlocator.cpp",
        "commandlocator.h",
        "directoryfilter.cpp",
        "directoryfilter.h",
        "directoryfilter.ui",
        "executefilter.cpp",
        "executefilter.h",
        "filesystemfilter.cpp",
        "filesystemfilter.h",
        "filesystemfilter.ui",
        "ilocatorfilter.cpp",
        "ilocatorfilter.h",
        "locator.qrc",
        "locator_global.h",
        "locatorconstants.h",
        "locatorfiltersfilter.cpp",
        "locatorfiltersfilter.h",
        "locatormanager.cpp",
        "locatormanager.h",
        "locatorplugin.cpp",
        "locatorplugin.h",
        "locatorwidget.cpp",
        "locatorwidget.h",
        "opendocumentsfilter.cpp",
        "opendocumentsfilter.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
        "images/locator.png",
        "images/reload.png",
    ]
}
