import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "ResourceEditor"

    Depends { name: "Core" }
    Depends { name: "Find" }
    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets", "xml"] }

    cpp.includePaths: [
        "..",
        "../..",
        "../../libs",
        buildDirectory,
        "qrceditor",
        "../../tools/utils"
    ]

    files: [
        "ResourceEditor.mimetypes.xml",
        "resourceeditor.qrc",
        "resourceeditorconstants.h",
        "resourceeditorfactory.cpp",
        "resourceeditorfactory.h",
        "resourceeditorplugin.cpp",
        "resourceeditorplugin.h",
        "resourceeditorw.cpp",
        "resourceeditorw.h",
        "resourcewizard.cpp",
        "resourcewizard.h",
        "qrceditor/resourcefile.cpp",
        "qrceditor/resourceview.cpp",
        "qrceditor/qrceditor.cpp",
        "qrceditor/undocommands.cpp",
        "qrceditor/resourcefile_p.h",
        "qrceditor/resourceview.h",
        "qrceditor/qrceditor.h",
        "qrceditor/undocommands_p.h",
        "qrceditor/qrceditor.ui"
    ]
}

