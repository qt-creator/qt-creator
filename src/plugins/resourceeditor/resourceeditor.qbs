QtcPlugin {
    name: "ResourceEditor"

    Depends { name: "Qt"; submodules: ["widgets", "xml"] }

    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    cpp.defines: base.concat(["RESOURCEEDITOR_LIBRARY"])

    Group {
        name: "General"
        files: [
            "resourceeditorconstants.h",
            "resourceeditorfactory.cpp", "resourceeditorfactory.h",
            "resourceeditorplugin.cpp", "resourceeditorplugin.h",
            "resourceeditortr.h",
            "resourceeditorw.cpp", "resourceeditorw.h",
            "resource_global.h", "resourcenode.cpp", "resourcenode.h"
        ]
    }

    Group {
        name: "QRC Editor"
        prefix: "qrceditor/"
        files: [
            "qrceditor.cpp", "qrceditor.h",
            "resourcefile.cpp", "resourcefile_p.h",
            "resourceview.cpp", "resourceview.h",
            "undocommands.cpp", "undocommands_p.h",
        ]
    }
}
