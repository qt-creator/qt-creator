import qbs 1.0

QtcLibrary {
    name: "ExtensionSystem"

    cpp.defines: base.concat([
        "EXTENSIONSYSTEM_LIBRARY",
        "IDE_TEST_DIR=\".\""
    ])

    Depends { name: "Qt"; submodules: ["core", "widgets"] }
    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    files: [
        "extensionsystem_global.h",
        "invoker.cpp",
        "invoker.h",
        "iplugin.cpp",
        "iplugin.h",
        "iplugin_p.h",
        "optionsparser.cpp",
        "optionsparser.h",
        "plugincollection.cpp",
        "plugincollection.h",
        "plugindetailsview.cpp",
        "plugindetailsview.h",
        "plugindetailsview.ui",
        "pluginerroroverview.cpp",
        "pluginerroroverview.h",
        "pluginerroroverview.ui",
        "pluginerrorview.cpp",
        "pluginerrorview.h",
        "pluginerrorview.ui",
        "pluginmanager.cpp",
        "pluginmanager.h",
        "pluginmanager_p.h",
        "pluginspec.cpp",
        "pluginspec.h",
        "pluginspec_p.h",
        "pluginview.cpp",
        "pluginview.h",
        "pluginview.qrc",
        "images/error.png",
        "images/notloaded.png",
        "images/ok.png",
    ]

    Export {
        Depends { name: "Qt.core" }
    }
}
