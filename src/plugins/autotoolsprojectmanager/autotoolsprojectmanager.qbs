import qbs 1.0

QtcPlugin {
    name: "AutotoolsProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "CppTools" }
    Depends { name: "QtSupport" }

    files: [
        "autogenstep.cpp",
        "autogenstep.h",
        "autoreconfstep.cpp",
        "autoreconfstep.h",
        "autotoolsbuildconfiguration.cpp",
        "autotoolsbuildconfiguration.h",
        "autotoolsbuildsystem.cpp",
        "autotoolsbuildsystem.h",
        "autotoolsopenprojectwizard.cpp",
        "autotoolsopenprojectwizard.h",
        "autotoolsprojectconstants.h",
        "autotoolsprojectplugin.cpp",
        "autotoolsprojectplugin.h",
        "configurestep.cpp",
        "configurestep.h",
        "makefileparser.cpp",
        "makefileparser.h",
        "makefileparserthread.cpp",
        "makefileparserthread.h",
        "makestep.cpp",
        "makestep.h",
    ]
}
