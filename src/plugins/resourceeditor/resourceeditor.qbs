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
            "resource_global.h",
            "resourceeditor.cpp",
            "resourceeditor.h",
            "resourceeditorconstants.h",
            "resourceeditorplugin.cpp",
            "resourceeditortr.h",
            "resourcenode.cpp",
            "resourcenode.h"
        ]
    }

    Group {
        name: "QRC Editor"
        prefix: "qrceditor/"
        files: [
            "qrceditor.cpp", "qrceditor.h",
            "resourcefile.cpp", "resourcefile_p.h",
        ]
    }
}
