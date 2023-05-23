import qbs 1.0

QtcPlugin {
    name: "IncrediBuild"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "buildconsolebuildstep.cpp",
        "buildconsolebuildstep.h",
        "cmakecommandbuilder.cpp",
        "cmakecommandbuilder.h",
        "commandbuilder.cpp",
        "commandbuilder.h",
        "commandbuilderaspect.cpp",
        "commandbuilderaspect.h",
        "ibconsolebuildstep.cpp",
        "ibconsolebuildstep.h",
        "incredibuild_global.h",
        "incredibuildconstants.h",
        "incredibuildplugin.cpp",
        "incredibuildtr.h",
        "makecommandbuilder.cpp",
        "makecommandbuilder.h",
    ]
}
