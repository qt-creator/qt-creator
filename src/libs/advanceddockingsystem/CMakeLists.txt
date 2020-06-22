add_qtc_library(AdvancedDockingSystem
  DEPENDS Qt5::Widgets Qt5::Core Qt5::Gui Utils
  SOURCES
    ads_globals.cpp ads_globals.h
    dockareatabbar.cpp dockareatabbar.h
    dockareatitlebar.cpp dockareatitlebar.h
    dockareawidget.cpp dockareawidget.h
    dockcomponentsfactory.cpp dockcomponentsfactory.h
    dockcontainerwidget.cpp dockcontainerwidget.h
    dockfocuscontroller.cpp dockfocuscontroller.h
    dockingstatereader.cpp dockingstatereader.h
    dockmanager.cpp dockmanager.h
    dockoverlay.cpp dockoverlay.h
    docksplitter.cpp docksplitter.h
    dockwidget.cpp dockwidget.h
    dockwidgettab.cpp dockwidgettab.h
    elidinglabel.cpp elidinglabel.h
    floatingdockcontainer.cpp floatingdockcontainer.h
    floatingdragpreview.cpp floatingdragpreview.h
    iconprovider.cpp iconprovider.h
    workspacedialog.cpp workspacedialog.h
    workspacemodel.cpp workspacemodel.h
    workspaceview.cpp workspaceview.h
    workspacedialog.ui
)

extend_qtc_library(AdvancedDockingSystem
  INCLUDES linux
  SOURCES
    linux/floatingwidgettitlebar.cpp linux/floatingwidgettitlebar.h
)
