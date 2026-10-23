import qbs 1.0

QtcPlugin {
    name: "Zephyr"

    Depends { name: "Qt"; submodules: ["widgets"] }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Debugger" }
    Depends { name: "QtSupport" }

    files: [
        "westbuildstep.cpp", "westbuildstep.h",
        "zephyrconstants.h",
        "zephyrdebug.cpp", "zephyrdebug.h",
        "zephyrplugin.cpp",
        "zephyrproject.cpp", "zephyrproject.h",
        "zephyrrun.cpp", "zephyrrun.h",
        "zephyrsettings.cpp", "zephyrsettings.h",
        "zephyrtr.h",
    ]

    Group {
        name: "wizards"
        files: [
            "wizards/application/CMakeLists.txt",
            "wizards/application/main.c",
            "wizards/application/prj.conf",
            "wizards/application/west.yml",
            "wizards/application/wizard.json",
        ]
        fileTags: "qt.core.resource_data"
    }
}
