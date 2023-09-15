import qbs 1.0

QtcLibrary {
    name: "AdvancedDockingSystem"

    cpp.optimization: "fast"
    cpp.defines: base.concat("ADVANCEDDOCKINGSYSTEM_LIBRARY")
    cpp.includePaths: base.concat([".", linux.prefix])

    Depends { name: "Qt"; submodules: ["quickwidgets", "widgets", "xml"] }
    Depends { name: "Utils" }

    Group {
        name: "General"
        files: [
            "ads_globals.cpp", "ads_globals.h",
            "advanceddockingsystemtr.h",
            "autohidedockcontainer.cpp", "autohidedockcontainer.h",
            "autohidesidebar.cpp", "autohidesidebar.h",
            "autohidetab.cpp", "autohidetab.h",
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
            "pushbutton.cpp", "pushbutton.h",
            "resizehandle.cpp", "resizehandle.h",
            "workspace.cpp", "workspace.h",
            "workspacedialog.cpp", "workspacedialog.h",
            "workspaceinputdialog.cpp", "workspaceinputdialog.h",
            "workspacemodel.cpp", "workspacemodel.h",
            "workspaceview.cpp", "workspaceview.h",
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
