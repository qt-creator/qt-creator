import qbs 1.0

QtcPlugin {
    name: "Coco"

    Depends { name: "Core" }
    Depends { name: "LanguageClient" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    Depends { name: "Qt"; submodules: ["widgets"] }

    files: [
        "buildsettings.cpp",
        "buildsettings.h",
        "cmakemodificationfile.cpp",
        "cmakemodificationfile.h",
        "cocobuildstep.cpp",
        "cocobuildstep.h",
        "cococommon.cpp",
        "cococommon.h",
        "cocolanguageclient.cpp",
        "cocolanguageclient.h",
        "cocoplugin.cpp",
        "cocoplugin_global.h",
        "cocopluginconstants.h",
        "cocoprojectwidget.cpp",
        "cocoprojectwidget.h",
        "cocotr.h",
        "globalsettings.cpp",
        "globalsettings.h",
        "modificationfile.cpp",
        "modificationfile.h",
        "qmakefeaturefile.cpp",
        "qmakefeaturefile.h",
    ]

    Group {
        name: "runtime resources"
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: "/cocoplugin"
        files: [
            "files/cocoplugin-clang.cmake",
            "files/cocoplugin-gcc.cmake",
            "files/cocoplugin-visualstudio.cmake",
            "files/cocoplugin.cmake",
            "files/cocoplugin.prf",
            "images/SquishCoco_48x48.png",
        ]
    }
}

