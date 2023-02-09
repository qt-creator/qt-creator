import qbs 1.0

Project {
    name: "ExtensionSystem"

    QtcLibrary {
        cpp.defines: base.concat([
            "EXTENSIONSYSTEM_LIBRARY",
            "IDE_TEST_DIR=\".\""
        ])

        Depends { name: "Qt"; submodules: ["core", "widgets"] }
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
            "pluginspec_p.h",
            "pluginview.cpp",
            "pluginview.h",
        ]

        Export {
            Depends { name: "Qt.core" }
        }
    }
}
