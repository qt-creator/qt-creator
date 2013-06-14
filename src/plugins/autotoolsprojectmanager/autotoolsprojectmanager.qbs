import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "AutotoolsProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "CppTools" }

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
