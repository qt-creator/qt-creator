import qbs.FileInfo

Project {
    name: "GNProjectManager"

    QtcPlugin {
        Depends { name: "Qt.widgets" }
        Depends { name: "Utils" }

        Depends { name: "Core" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "QtSupport" }

        cpp.includePaths: "."

        files: [
            "gnactionsmanager.cpp",
            "gnactionsmanager.h",
            "gnbuildconfiguration.cpp",
            "gnbuildconfiguration.h",
            "gnbuildstep.cpp",
            "gnbuildstep.h",
            "gnbuildsystem.cpp",
            "gnbuildsystem.h",
            "gninfoparser.cpp",
            "gninfoparser.h",
            "gnkitaspect.cpp",
            "gnkitaspect.h",
            "gnpluginconstants.h",
            "gnproject.cpp",
            "gnproject.h",
            "gnprojectmanagertr.h",
            "gnprojectnodes.cpp",
            "gnprojectnodes.h",
            "gnprojectparser.cpp",
            "gnprojectparser.h",
            "gnprojectplugin.cpp",
            "gntarget.h",
            "gntools.cpp",
            "gntools.h",
            "gntoolssettingsaccessor.cpp",
            "gntoolssettingsaccessor.h",
            "gntoolssettingspage.cpp",
            "gntoolssettingspage.h",
            "gnutils.cpp",
            "gnutils.h",
            "ninjaparser.cpp",
            "ninjaparser.h",
        ]
    }
}
