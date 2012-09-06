TEMPLATE = lib
TARGET = Qt4ProjectManager
DEFINES += QT_CREATOR QT4PROJECTMANAGER_LIBRARY
QT += network
include(../../qtcreatorplugin.pri)
include(qt4projectmanager_dependencies.pri)

HEADERS += \
    qmakekitinformation.h \
    qmakekitconfigwidget.h \
    qmakerunconfigurationfactory.h \
    qt4projectmanagerplugin.h \
    qt4projectmanager.h \
    qt4project.h \
    qt4nodes.h \
    profileeditor.h \
    profilehighlighter.h \
    profileeditorfactory.h \
    profilehoverhandler.h \
    wizards/qtprojectparameters.h \
    wizards/guiappwizard.h \
    wizards/mobileapp.h \
    wizards/mobileappwizard.h \
    wizards/mobileappwizardpages.h \
    wizards/mobilelibrarywizardoptionpage.h \
    wizards/mobilelibraryparameters.h \
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
    wizards/targetsetuppage.h \
    wizards/importwidget.h \
    wizards/qtquickapp.h \
    wizards/qtquickappwizard.h \
    wizards/qtquickappwizardpages.h \
    wizards/html5app.h \
    wizards/html5appwizard.h \
    wizards/html5appwizardpages.h \
    wizards/abstractmobileapp.h \
    wizards/abstractmobileappwizard.h \
    wizards/subdirsprojectwizard.h \
    wizards/subdirsprojectwizarddialog.h \
    qt4projectmanagerconstants.h \
    makestep.h \
    qmakestep.h \
    qtmodulesinfo.h \
    qt4projectconfigwidget.h \
    qtuicodemodelsupport.h \
    externaleditors.h \
    qt4buildconfiguration.h \
    qmakeparser.h \
    addlibrarywizard.h \
    librarydetailscontroller.h \
    findqt4profiles.h \
    qt4projectmanager_global.h \
    qt4targetsetupwidget.h \
    buildconfigurationinfo.h \
    winceqtversionfactory.h \
    winceqtversion.h \
    profilecompletionassist.h \
    unconfiguredprojectpanel.h

SOURCES += \
    qmakekitconfigwidget.cpp \
    qmakekitinformation.cpp \
    qmakerunconfigurationfactory.cpp \
    qt4projectmanagerplugin.cpp \
    qt4projectmanager.cpp \
    qt4project.cpp \
    qt4nodes.cpp \
    profileeditor.cpp \
    profilehighlighter.cpp \
    profileeditorfactory.cpp \
    profilehoverhandler.cpp \
    wizards/qtprojectparameters.cpp \
    wizards/guiappwizard.cpp \
    wizards/mobileapp.cpp \
    wizards/mobileappwizard.cpp \
    wizards/mobileappwizardpages.cpp \
    wizards/mobilelibrarywizardoptionpage.cpp \
    wizards/mobilelibraryparameters.cpp \
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
    wizards/targetsetuppage.cpp \
    wizards/importwidget.cpp \
    wizards/qtquickapp.cpp \
    wizards/qtquickappwizard.cpp \
    wizards/qtquickappwizardpages.cpp \
    wizards/html5app.cpp \
    wizards/html5appwizard.cpp \
    wizards/html5appwizardpages.cpp \
    wizards/abstractmobileapp.cpp \
    wizards/abstractmobileappwizard.cpp \
    wizards/subdirsprojectwizard.cpp \
    wizards/subdirsprojectwizarddialog.cpp \
    makestep.cpp \
    qmakestep.cpp \
    qtmodulesinfo.cpp \
    qt4projectconfigwidget.cpp \
    qtuicodemodelsupport.cpp \
    externaleditors.cpp \
    qt4buildconfiguration.cpp \
    qmakeparser.cpp \
    addlibrarywizard.cpp \
    librarydetailscontroller.cpp \
    findqt4profiles.cpp \
    qt4targetsetupwidget.cpp \
    winceqtversionfactory.cpp \
    winceqtversion.cpp \
    profilecompletionassist.cpp \
    unconfiguredprojectpanel.cpp

FORMS += makestep.ui \
    qmakestep.ui \
    qt4projectconfigwidget.ui \
    librarydetailswidget.ui \
    wizards/testwizardpage.ui \
    wizards/html5appwizardsourcespage.ui \
    wizards/mobilelibrarywizardoptionpage.ui \
    wizards/mobileappwizardgenericoptionspage.ui \
    wizards/mobileappwizardmaemooptionspage.ui \
    wizards/mobileappwizardharmattanoptionspage.ui \
    wizards/qtquickcomponentsetoptionspage.ui

RESOURCES += qt4projectmanager.qrc \
    wizards/wizards.qrc

include(qt-desktop/qt-desktop.pri)
include(customwidgetwizard/customwidgetwizard.pri)

DEFINES += QT_NO_CAST_TO_ASCII
OTHER_FILES += Qt4ProjectManager.mimetypes.xml
