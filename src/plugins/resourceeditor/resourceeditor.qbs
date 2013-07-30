import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "ResourceEditor"

    Depends { name: "Core" }
    Depends { name: "Find" }
    Depends { name: "Qt"; submodules: ["widgets", "xml"] }

    cpp.includePaths: base.concat(qrcEdtitor.prefix)

    Group {
        name: "General"
        files: [
            "resourceeditor.qrc",
            "resourceeditorconstants.h",
            "resourceeditorfactory.cpp", "resourceeditorfactory.h",
            "resourceeditorplugin.cpp", "resourceeditorplugin.h",
            "resourceeditorw.cpp", "resourceeditorw.h",
            "resourcewizard.cpp", "resourcewizard.h",
        ]
    }

    Group {
        name: "QRC Editor"
        id: qrcEdtitor
        prefix: "qrceditor/"
        files: [
            "qrceditor.cpp", "qrceditor.h", "qrceditor.ui",
            "resourcefile.cpp", "resourcefile_p.h",
            "resourceview.cpp", "resourceview.h",
            "undocommands.cpp", "undocommands_p.h",
        ]
    }
}
