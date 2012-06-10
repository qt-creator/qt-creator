import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "AutotoolsProjectManager"

    Depends { name: "qt"; submodules: ['widgets'] }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "CppTools" }

    Depends { name: "cpp" }
    cpp.defines: base.concat(["QT_NO_CAST_FROM_ASCII"])
    cpp.includePaths: [
        ".",
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "autogenstep.cpp",
        "autogenstep.h",
        "autoreconfstep.cpp",
        "autoreconfstep.h",
        "autotoolsbuildconfiguration.cpp",
        "autotoolsbuildconfiguration.h",
        "autotoolsbuildsettingswidget.cpp",
        "autotoolsbuildsettingswidget.h",
        "autotoolsmanager.cpp",
        "autotoolsmanager.h",
        "autotoolsopenprojectwizard.cpp",
        "autotoolsopenprojectwizard.h",
        "autotoolsproject.cpp",
        "autotoolsproject.h",
        "autotoolsproject.qrc",
        "autotoolsprojectconstants.h",
        "autotoolsprojectfile.cpp",
        "autotoolsprojectfile.h",
        "autotoolsprojectnode.cpp",
        "autotoolsprojectnode.h",
        "autotoolsprojectplugin.cpp",
        "autotoolsprojectplugin.h",
        "autotoolstarget.cpp",
        "autotoolstarget.h",
        "configurestep.cpp",
        "configurestep.h",
        "makefileparser.cpp",
        "makefileparser.h",
        "makefileparserthread.cpp",
        "makefileparserthread.h",
        "makestep.cpp",
        "makestep.h"
    ]
}
