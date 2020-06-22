import qbs 1.0

QtcLibrary {
    name: "AdvancedDockingSystem"

    cpp.optimization: "fast"
    cpp.defines: base.concat("ADVANCEDDOCKINGSYSTEM_LIBRARY")
    cpp.includePaths: base.concat([".", linux.prefix])

    Depends { name: "Qt"; submodules: ["widgets", "core", "gui"] }
    Depends { name: "Utils" }

    Group {
        name: "General"
        files: [
            "ads_globals.cpp", "ads_globals.h",
            "dockareatabbar.cpp", "dockareatabbar.h",
            "dockareatitlebar.cpp", "dockareatitlebar.h",
            "dockareawidget.cpp", "dockareawidget.h",
            "dockcomponentsfactory.cpp", "dockcomponentsfactory.h",
            "dockcontainerwidget.cpp", "dockcontainerwidget.h",
            "dockfocuscontroller.cpp", "dockfocuscontroller.h",
            "dockingstatereader.cpp", "dockingstatereader.h",
            "dockmanager.cpp", "dockmanager.h",
            "dockoverlay.cpp", "dockoverlay.h",
            "docksplitter.cpp", "docksplitter.h",
            "dockwidget.cpp", "dockwidget.h",
            "dockwidgettab.cpp", "dockwidgettab.h",
            "elidinglabel.cpp", "elidinglabel.h",
            "floatingdockcontainer.cpp", "floatingdockcontainer.h",
            "floatingdragpreview.cpp", "floatingdragpreview.h",
            "iconprovider.cpp", "iconprovider.h",
            "workspacedialog.cpp", "workspacedialog.h",
            "workspacemodel.cpp", "workspacemodel.h",
            "workspaceview.cpp", "workspaceview.h",
            "workspacedialog.ui"
        ]
    }

    Group {
        name: "Linux"
        id: linux
        prefix: "linux/"
        files: [
            "floatingwidgettitlebar.cpp", "floatingwidgettitlebar.h"
        ]
    }
}
