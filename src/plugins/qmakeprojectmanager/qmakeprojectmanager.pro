QT += network
include(../../qtcreatorplugin.pri)

DEFINES += \
    QMAKEPROJECTMANAGER_LIBRARY

HEADERS += \
    androidextralibrarylistmodel.h \
    androidpackageinstallationfactory.h \
    androidpackageinstallationstep.h \
    createandroidmanifestwizard.h \
    qmakebuildinfo.h \
    qmakekitinformation.h \
    qmakekitconfigwidget.h \
    qmakeprojectimporter.h \
    qmakerunconfigurationfactory.h \
    qmakeprojectmanagerplugin.h \
    qmakeprojectmanager.h \
    qmakeproject.h \
    qmakenodes.h \
    profileeditor.h \
    profilehighlighter.h \
    profileeditorfactory.h \
    profilehoverhandler.h \
    wizards/qtprojectparameters.h \
    wizards/guiappwizard.h \
    wizards/consoleappwizard.h \
    wizards/consoleappwizarddialog.h \
    wizards/libraryparameters.h \
    wizards/librarywizard.h \
    wizards/librarywizarddialog.h \
    wizards/guiappwizarddialog.h \
    wizards/emptyprojectwizard.h \
    wizards/emptyprojectwizarddialog.h \
    wizards/testwizard.h \
    wizards/testwizarddialog.h \
    wizards/testwizardpage.h \
    wizards/modulespage.h \
    wizards/filespage.h \
    wizards/qtwizard.h \
    wizards/qtquickapp.h \
    wizards/qtquickappwizard.h \
    wizards/qtquickappwizardpages.h \
    wizards/abstractmobileapp.h \
    wizards/abstractmobileappwizard.h \
    wizards/subdirsprojectwizard.h \
    wizards/subdirsprojectwizarddialog.h \
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
    qmakeandroidsupport.h \
    qmakeandroidrunconfiguration.h \
    qmakeandroidrunfactories.h \
    qmakeandroidbuildapkstep.h \
    qmakeandroidbuildapkwidget.h \
    androidqmakebuildconfigurationfactory.h

SOURCES += \
    androidextralibrarylistmodel.cpp \
    androidpackageinstallationfactory.cpp \
    androidpackageinstallationstep.cpp \
    createandroidmanifestwizard.cpp \
    qmakekitconfigwidget.cpp \
    qmakekitinformation.cpp \
    qmakeprojectimporter.cpp \
    qmakerunconfigurationfactory.cpp \
    qmakeprojectmanagerplugin.cpp \
    qmakeprojectmanager.cpp \
    qmakeproject.cpp \
    qmakenodes.cpp \
    profileeditor.cpp \
    profilehighlighter.cpp \
    profileeditorfactory.cpp \
    profilehoverhandler.cpp \
    wizards/qtprojectparameters.cpp \
    wizards/guiappwizard.cpp \
    wizards/consoleappwizard.cpp \
    wizards/consoleappwizarddialog.cpp \
    wizards/libraryparameters.cpp \
    wizards/librarywizard.cpp \
    wizards/librarywizarddialog.cpp \
    wizards/guiappwizarddialog.cpp \
    wizards/emptyprojectwizard.cpp \
    wizards/emptyprojectwizarddialog.cpp \
    wizards/testwizard.cpp \
    wizards/testwizarddialog.cpp \
    wizards/testwizardpage.cpp \
    wizards/modulespage.cpp \
    wizards/filespage.cpp \
    wizards/qtwizard.cpp \
    wizards/qtquickapp.cpp \
    wizards/qtquickappwizard.cpp \
    wizards/qtquickappwizardpages.cpp \
    wizards/abstractmobileapp.cpp \
    wizards/abstractmobileappwizard.cpp \
    wizards/subdirsprojectwizard.cpp \
    wizards/subdirsprojectwizarddialog.cpp \
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
    qmakeandroidsupport.cpp \
    qmakeandroidrunconfiguration.cpp \
    qmakeandroidrunfactories.cpp \
    qmakeandroidbuildapkstep.cpp \
    qmakeandroidbuildapkwidget.cpp \
    androidqmakebuildconfigurationfactory.cpp

FORMS += makestep.ui \
    qmakestep.ui \
    qmakeprojectconfigwidget.ui \
    librarydetailswidget.ui \
    wizards/testwizardpage.ui \
    qmakeandroidbuildapkwidget.ui

RESOURCES += qmakeprojectmanager.qrc \
    wizards/wizards.qrc

include(customwidgetwizard/customwidgetwizard.pri)
