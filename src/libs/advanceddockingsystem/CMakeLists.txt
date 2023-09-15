add_qtc_library(AdvancedDockingSystem
  CONDITION TARGET Qt::QuickWidgets
  DEPENDS Qt::Widgets Qt::Core Qt::Gui Qt::Xml Qt::QuickWidgets Utils
  SOURCES
    ads_globals.cpp ads_globals.h
    advanceddockingsystemtr.h
    autohidedockcontainer.cpp autohidedockcontainer.h
    autohidesidebar.cpp autohidesidebar.h
    autohidetab.cpp autohidetab.h
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
    pushbutton.cpp pushbutton.h
    resizehandle.cpp resizehandle.h
    workspace.cpp workspace.h
    workspacedialog.cpp workspacedialog.h
    workspaceinputdialog.cpp workspaceinputdialog.h
    workspacemodel.cpp workspacemodel.h
    workspaceview.cpp workspaceview.h
)

extend_qtc_library(AdvancedDockingSystem
  INCLUDES linux
  CONDITION UNIX AND NOT APPLE
  SOURCES
    linux/floatingwidgettitlebar.cpp linux/floatingwidgettitlebar.h
)
