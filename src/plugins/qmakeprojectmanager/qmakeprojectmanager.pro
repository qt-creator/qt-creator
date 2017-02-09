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
    qmakerunconfigurationfactory.h \
    qmakeprojectmanagerplugin.h \
    qmakeprojectmanager.h \
    qmakeproject.h \
    qmakenodes.h \
    profileeditor.h \
    profilehighlighter.h \
    profilehoverhandler.h \
    wizards/qtprojectparameters.h \
    wizards/guiappwizard.h \
    wizards/libraryparameters.h \
    wizards/librarywizard.h \
    wizards/librarywizarddialog.h \
    wizards/guiappwizarddialog.h \
    wizards/testwizard.h \
    wizards/testwizarddialog.h \
    wizards/testwizardpage.h \
    wizards/modulespage.h \
    wizards/filespage.h \
    wizards/qtwizard.h \
    wizards/subdirsprojectwizard.h \
    wizards/subdirsprojectwizarddialog.h \
    wizards/simpleprojectwizard.h \
    qmakeprojectmanagerconstants.h \
    makestep.h \
    qmakestep.h \
    qtmodulesinfo.h \
    qmakeprojectconfigwidget.h \
    externaleditors.h \
    qmakebuildconfiguration.h \
    qmakeparser.h \
    addlibrarywizard.h \
    librarydetailscontroller.h \
    findqmakeprofiles.h \
    qmakeprojectmanager_global.h \
    desktopqmakerunconfiguration.h \
    profilecompletionassist.h \
    makefileparse.h

SOURCES += \
    qmakekitconfigwidget.cpp \
    qmakekitinformation.cpp \
    qmakeparsernodes.cpp \
    qmakeprojectimporter.cpp \
    qmakerunconfigurationfactory.cpp \
    qmakeprojectmanagerplugin.cpp \
    qmakeprojectmanager.cpp \
    qmakeproject.cpp \
    qmakenodes.cpp \
    profileeditor.cpp \
    profilehighlighter.cpp \
    profilehoverhandler.cpp \
    wizards/qtprojectparameters.cpp \
    wizards/guiappwizard.cpp \
    wizards/libraryparameters.cpp \
    wizards/librarywizard.cpp \
    wizards/librarywizarddialog.cpp \
    wizards/guiappwizarddialog.cpp \
    wizards/testwizard.cpp \
    wizards/testwizarddialog.cpp \
    wizards/testwizardpage.cpp \
    wizards/modulespage.cpp \
    wizards/filespage.cpp \
    wizards/qtwizard.cpp \
    wizards/subdirsprojectwizard.cpp \
    wizards/subdirsprojectwizarddialog.cpp \
    wizards/simpleprojectwizard.cpp \
    makestep.cpp \
    qmakestep.cpp \
    qtmodulesinfo.cpp \
    qmakeprojectconfigwidget.cpp \
    externaleditors.cpp \
    qmakebuildconfiguration.cpp \
    qmakeparser.cpp \
    addlibrarywizard.cpp \
    librarydetailscontroller.cpp \
    findqmakeprofiles.cpp \
    desktopqmakerunconfiguration.cpp \
    profilecompletionassist.cpp \
    makefileparse.cpp

FORMS += makestep.ui \
    qmakestep.ui \
    qmakeprojectconfigwidget.ui \
    librarydetailswidget.ui \
    wizards/testwizardpage.ui

RESOURCES += qmakeprojectmanager.qrc \
    wizards/wizards.qrc

include(customwidgetwizard/customwidgetwizard.pri)
