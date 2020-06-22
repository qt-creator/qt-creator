shared {
    DEFINES += ADVANCEDDOCKINGSYSTEM_LIBRARY
} else {
    DEFINES += BUILD_ADVANCEDDOCKINGSYSTEM_STATIC_LIB
}

## Input
HEADERS += \
    ads_globals.h \
    dockareatabbar.h \
    dockareatitlebar.h \
    dockareawidget.h \
    dockcomponentsfactory.h \
    dockcontainerwidget.h \
    dockfocuscontroller.h \
    dockingstatereader.h \
    dockmanager.h \
    dockoverlay.h \
    docksplitter.h \
    dockwidget.h \
    dockwidgettab.h \
    elidinglabel.h \
    floatingdockcontainer.h \
    floatingdragpreview.h \
    iconprovider.h \
    workspacedialog.h \
    workspacemodel.h \
    workspaceview.h

SOURCES += \
    ads_globals.cpp \
    dockareatabbar.cpp \
    dockareatitlebar.cpp \
    dockareawidget.cpp \
    dockcomponentsfactory.cpp \
    dockcontainerwidget.cpp \
    dockfocuscontroller.cpp \
    dockingstatereader.cpp \
    dockmanager.cpp \
    dockoverlay.cpp \
    docksplitter.cpp \
    dockwidget.cpp \
    dockwidgettab.cpp \
    elidinglabel.cpp \
    floatingdockcontainer.cpp \
    floatingdragpreview.cpp \
    iconprovider.cpp \
    workspacedialog.cpp \
    workspacemodel.cpp \
    workspaceview.cpp

FORMS += \
        workspacedialog.ui

include(linux/linux.pri)

DISTFILES += advanceddockingsystem.pri
