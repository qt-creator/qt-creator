import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "ResourceEditor"

    Depends { name: "Core" }
    Depends { name: "Find" }
    Depends { name: "Qt"; submodules: ["widgets", "xml"] }

    cpp.includePaths: base.concat("qrceditor")

    files: [
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
        "qrceditor/qrceditor.cpp",
        "qrceditor/qrceditor.h",
        "qrceditor/qrceditor.ui",
        "qrceditor/resourcefile.cpp",
        "qrceditor/resourcefile_p.h",
        "qrceditor/resourceview.cpp",
        "qrceditor/resourceview.h",
        "qrceditor/undocommands.cpp",
        "qrceditor/undocommands_p.h",
    ]
}
