import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "ResourceEditor"

    Depends { name: "Qt"; submodules: ["widgets", "xml"] }
    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }

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
        prefix: "qrceditor/"
        files: [
            "qrceditor.cpp", "qrceditor.h", "qrceditor.ui",
            "resourcefile.cpp", "resourcefile_p.h",
            "resourceview.cpp", "resourceview.h",
            "undocommands.cpp", "undocommands_p.h",
        ]
    }
}
