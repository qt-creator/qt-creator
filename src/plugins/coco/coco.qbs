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
        "cococmakesettings.cpp",
        "cococmakesettings.h",
        "cococommon.cpp",
        "cococommon.h",
        "cocoinstallation.cpp",
        "cocoinstallation.h",
        "cocolanguageclient.cpp",
        "cocolanguageclient.h",
        "cocoplugin.cpp",
        "cocoplugin.qrc",
        "cocoplugin_global.h",
        "cocopluginconstants.h",
        "cocoprojectsettingswidget.cpp",
        "cocoprojectsettingswidget.h",
        "cocoprojectwidget.cpp",
        "cocoprojectwidget.h",
        "cocoqmakesettings.cpp",
        "cocoqmakesettings.h",
        "cocotr.h",
        "files/cocoplugin-clang.cmake",
        "files/cocoplugin-gcc.cmake",
        "files/cocoplugin-visualstudio.cmake",
        "files/cocoplugin.cmake",
        "files/cocoplugin.prf",
        "globalsettings.cpp",
        "globalsettings.h",
        "globalsettingspage.cpp",
        "globalsettingspage.h",
        "images/SquishCoco_48x48.png",
        "modificationfile.cpp",
        "modificationfile.h",
        "qmakefeaturefile.cpp",
        "qmakefeaturefile.h",
    ]
}

