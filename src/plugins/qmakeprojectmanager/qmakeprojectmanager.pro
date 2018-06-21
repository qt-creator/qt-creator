QT += network
include(../../qtcreatorplugin.pri)

DEFINES += \
    QMAKEPROJECTMANAGER_LIBRARY

HEADERS += \
    qmakebuildinfo.h \
    qmakekitinformation.h \
    qmakekitconfigwidget.h \
    qmakeparsernodes.h \
    qmakeprojectimporter.h \
    qmakeprojectmanagerplugin.h \
    qmakeprojectmanager.h \
    qmakeproject.h \
    qmakenodes.h \
    qmakenodetreebuilder.h \
    profileeditor.h \
    profilehighlighter.h \
    profilehoverhandler.h \
    wizards/qtprojectparameters.h \
    wizards/guiappwizard.h \
    wizards/libraryparameters.h \
    wizards/librarywizard.h \
    wizards/librarywizarddialog.h \
    wizards/guiappwizarddialog.h \
    wizards/modulespage.h \
    wizards/filespage.h \
    wizards/qtwizard.h \
    wizards/subdirsprojectwizard.h \
    wizards/subdirsprojectwizarddialog.h \
    wizards/simpleprojectwizard.h \
    qmakeprojectmanagerconstants.h \
    qmakestep.h \
    qtmodulesinfo.h \
    qmakeprojectconfigwidget.h \
    externaleditors.h \
    qmakebuildconfiguration.h \
    qmakeparser.h \
    addlibrarywizard.h \
    librarydetailscontroller.h \
    qmakeprojectmanager_global.h \
    desktopqmakerunconfiguration.h \
    profilecompletionassist.h \
    makefileparse.h \
    qmakemakestep.h

SOURCES += \
    qmakekitconfigwidget.cpp \
    qmakekitinformation.cpp \
    qmakeparsernodes.cpp \
    qmakeprojectimporter.cpp \
    qmakeprojectmanagerplugin.cpp \
    qmakeprojectmanager.cpp \
    qmakeproject.cpp \
    qmakenodes.cpp \
    qmakenodetreebuilder.cpp \
    profileeditor.cpp \
    profilehighlighter.cpp \
    profilehoverhandler.cpp \
    wizards/qtprojectparameters.cpp \
    wizards/guiappwizard.cpp \
    wizards/libraryparameters.cpp \
    wizards/librarywizard.cpp \
    wizards/librarywizarddialog.cpp \
    wizards/guiappwizarddialog.cpp \
    wizards/modulespage.cpp \
    wizards/filespage.cpp \
    wizards/qtwizard.cpp \
    wizards/subdirsprojectwizard.cpp \
    wizards/subdirsprojectwizarddialog.cpp \
    wizards/simpleprojectwizard.cpp \
    qmakestep.cpp \
    qtmodulesinfo.cpp \
    qmakeprojectconfigwidget.cpp \
    externaleditors.cpp \
    qmakebuildconfiguration.cpp \
    qmakeparser.cpp \
    addlibrarywizard.cpp \
    librarydetailscontroller.cpp \
    desktopqmakerunconfiguration.cpp \
    profilecompletionassist.cpp \
    makefileparse.cpp \
    qmakemakestep.cpp

FORMS += \
    qmakestep.ui \
    qmakeprojectconfigwidget.ui \
    librarydetailswidget.ui

RESOURCES += qmakeprojectmanager.qrc \
    wizards/wizards.qrc

include(customwidgetwizard/customwidgetwizard.pri)
