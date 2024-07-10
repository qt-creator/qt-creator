QtcLibrary {
    name: "ExtensionSystem"

    cpp.defines: base.concat(["EXTENSIONSYSTEM_LIBRARY", "IDE_TEST_DIR=\".\""])
                     .concat(qtc.withPluginTests ? ["EXTENSIONSYSTEM_WITH_TESTOPTION"] : [])

    Depends { name: "Qt"; submodules: ["core", "widgets"] }
    Depends { name: "Qt.testlib"; condition: qtc.withPluginTests }

    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    files: [
        "extensionsystem_global.h",
        "extensionsystemtr.h",
        "invoker.cpp",
        "invoker.h",
        "iplugin.cpp",
        "iplugin.h",
        "optionsparser.cpp",
        "optionsparser.h",
        "plugindetailsview.cpp",
        "plugindetailsview.h",
        "pluginerroroverview.cpp",
        "pluginerroroverview.h",
        "pluginerrorview.cpp",
        "pluginerrorview.h",
        "pluginmanager.cpp",
        "pluginmanager.h",
        "pluginmanager_p.h",
        "pluginspec.cpp",
        "pluginspec.h",
        "pluginview.cpp",
        "pluginview.h",
    ]

    Export {
        Depends { name: "Qt.core" }
        Depends { name: "qtc" }
        cpp.defines: qtc.withPluginTests ? ["EXTENSIONSYSTEM_WITH_TESTOPTION"] : []
    }
}
