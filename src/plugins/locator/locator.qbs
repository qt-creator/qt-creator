import qbs.base 1.0
import qbs.FileInfo

import QtcPlugin

QtcPlugin {
    name: "Locator"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "script"] }
    Depends { name: "Core" }

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
        "locatorsearchutils.cpp",
        "locatorsearchutils.h",
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

    Group {
        name: "Tests"
        condition: project.testsEnabled
        files: [
            "locatorfiltertest.cpp",
            "locatorfiltertest.h",
            "locator_test.cpp"
        ]

        cpp.defines: outer.concat(['SRCDIR="' + FileInfo.path(filePath) + '"'])
    }
}
