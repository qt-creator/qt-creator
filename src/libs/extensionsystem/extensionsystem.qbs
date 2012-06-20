import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "ExtensionSystem"

    cpp.includePaths: [
        ".",
        ".."
    ]
    cpp.defines: [
        "EXTENSIONSYSTEM_LIBRARY",
        "IDE_TEST_DIR=\".\""
    ]

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["core", "widgets"] }
    Depends { name: "Aggregation" }

    files: [
        "plugindetailsview.ui",
        "pluginerrorview.ui",
        "pluginview.qrc",
        "pluginview.ui",
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
        "pluginerroroverview.cpp",
        "pluginerroroverview.h",
        "pluginerroroverview.ui",
        "pluginerrorview.cpp",
        "pluginerrorview.h",
        "pluginmanager.cpp",
        "pluginmanager.h",
        "pluginmanager_p.h",
        "pluginspec.h",
        "pluginspec_p.h",
        "pluginview.cpp",
        "pluginview.h",
        "pluginspec.cpp",
        "images/error.png",
        "images/notloaded.png",
        "images/ok.png",
    ]
}
