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
        "../../shared/qrceditor",
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
        "../../shared/qrceditor/resourcefile.cpp",
        "../../shared/qrceditor/resourceview.cpp",
        "../../shared/qrceditor/qrceditor.cpp",
        "../../shared/qrceditor/undocommands.cpp",
        "../../shared/qrceditor/resourcefile_p.h",
        "../../shared/qrceditor/resourceview.h",
        "../../shared/qrceditor/qrceditor.h",
        "../../shared/qrceditor/undocommands_p.h",
        "../../shared/qrceditor/qrceditor.ui"
    ]
}

