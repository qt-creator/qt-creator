import qbs 1.0

QtcPlugin {
    name: "Coco"

    Depends { name: "Core" }
    Depends { name: "LanguageClient" }
    Depends { name: "CMakeProjectManager" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmakeProjectManager" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    Depends { name: "Qt"; submodules: ["widgets"] }

    files: [
        "cocobuild/buildsettings.cpp",
        "cocobuild/buildsettings.h",
        "cocobuild/cmakemodificationfile.cpp",
        "cocobuild/cmakemodificationfile.h",
        "cocobuild/cocobuildstep.cpp",
        "cocobuild/cocobuildstep.h",
        "cocobuild/cococmakesettings.cpp",
        "cocobuild/cococmakesettings.h",
        "cocobuild/cocoprojectwidget.cpp",
        "cocobuild/cocoprojectwidget.h",
        "cocobuild/cocoprojectwidget.ui",
        "cocobuild/cocoqmakesettings.cpp",
        "cocobuild/cocoqmakesettings.h",
        "cocobuild/modificationfile.cpp",
        "cocobuild/modificationfile.h",
        "cocobuild/qmakefeaturefile.cpp",
        "cocobuild/qmakefeaturefile.h",
        "cocolanguageclient.cpp",
        "cocolanguageclient.h",
        "cocoplugin.cpp",
        "cocoplugin.qrc",
        "cocoplugin_global.h",
        "cocopluginconstants.h",
        "cocotr.h",
        "common.cpp",
        "common.h",
        "files/cocoplugin-clang.cmake",
        "files/cocoplugin-gcc.cmake",
        "files/cocoplugin-visualstudio.cmake",
        "files/cocoplugin.cmake",
        "files/cocoplugin.prf",
        "images/SquishCoco_48x48.png",
        "settings/cocoinstallation.cpp",
        "settings/cocoinstallation.h",
        "settings/cocoprojectsettingswidget.cpp",
        "settings/cocoprojectsettingswidget.h",
        "settings/globalsettings.cpp",
        "settings/globalsettings.h",
        "settings/globalsettingspage.cpp",
        "settings/globalsettingspage.h",
        "settings/globalsettingspage.ui",
    ]
}

