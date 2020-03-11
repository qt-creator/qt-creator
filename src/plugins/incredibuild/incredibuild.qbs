import qbs 1.0

QtcPlugin {
    name: "IncrediBuild"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "buildconsolebuildstep.cpp",
        "buildconsolebuildstep.h",
        "buildconsolebuildstep.ui",
        "buildconsolestepconfigwidget.cpp",
        "buildconsolestepconfigwidget.h",
        "buildconsolestepfactory.cpp",
        "buildconsolestepfactory.h",
        "cmakecommandbuilder.cpp",
        "cmakecommandbuilder.h",
        "commandbuilder.cpp",
        "commandbuilder.h",
        "ibconsolebuildstep.cpp",
        "ibconsolebuildstep.h",
        "ibconsolebuildstep.ui",
        "ibconsolestepconfigwidget.cpp",
        "ibconsolestepconfigwidget.h",
        "ibconsolestepfactory.cpp",
        "ibconsolestepfactory.h",
        "incredibuild_global.h",
        "incredibuildconstants.h",
        "incredibuildplugin.cpp",
        "incredibuildplugin.h",
        "makecommandbuilder.cpp",
        "makecommandbuilder.h",
    ]
}
