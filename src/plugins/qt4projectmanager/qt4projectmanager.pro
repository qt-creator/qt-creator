TEMPLATE = lib
TARGET = Qt4ProjectManager
QT += network
include(../../qtcreatorplugin.pri)
include(qt4projectmanager_dependencies.pri)
HEADERS += qt4projectmanagerplugin.h \
    qt4projectmanager.h \
    qt4project.h \
    qt4nodes.h \
    profileeditor.h \
    profilehighlighter.h \
    profileeditorfactory.h \
    profilereader.h \
    profilecache.h \
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
    wizards/modulespage.h \
    wizards/filespage.h \
    wizards/qtwizard.h \
    qt4projectmanagerconstants.h \
    makestep.h \
    qmakestep.h \
    deployhelper.h \
    embeddedpropertiespage.h \
    qt4runconfiguration.h \
    qtmodulesinfo.h \
    qt4projectconfigwidget.h \
    qt4buildenvironmentwidget.h \
    projectloadwizard.h \
    qtversionmanager.h \
    qtoptionspage.h \
    qtuicodemodelsupport.h \
    externaleditors.h \
    gettingstartedwelcomepagewidget.h \
    gettingstartedwelcomepage.h
SOURCES += qt4projectmanagerplugin.cpp \
    qt4projectmanager.cpp \
    qt4project.cpp \
    qt4nodes.cpp \
    profileeditor.cpp \
    profilehighlighter.cpp \
    profileeditorfactory.cpp \
    profilereader.cpp \
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
    wizards/modulespage.cpp \
    wizards/filespage.cpp \
    wizards/qtwizard.cpp \
    makestep.cpp \
    qmakestep.cpp \
    deployhelper.cpp \
    embeddedpropertiespage.cpp \
    qt4runconfiguration.cpp \
    qtmodulesinfo.cpp \
    qt4projectconfigwidget.cpp \
    qt4buildenvironmentwidget.cpp \
    projectloadwizard.cpp \
    qtversionmanager.cpp \
    qtoptionspage.cpp \
    qtuicodemodelsupport.cpp \
    externaleditors.cpp \
    gettingstartedwelcomepagewidget.cpp \
    gettingstartedwelcomepage.cpp
FORMS += makestep.ui \
    qmakestep.ui \
    qt4projectconfigwidget.ui \
    embeddedpropertiespage.ui \
    qtversionmanager.ui \
    showbuildlog.ui \
    gettingstartedwelcomepagewidget.ui
RESOURCES += qt4projectmanager.qrc \
    wizards/wizards.qrc
include(../../shared/proparser/proparser.pri)
include(qt-s60/qt-s60.pri)
include(qt-maemo/qt-maemo.pri)
include(customwidgetwizard/customwidgetwizard.pri)
DEFINES += QT_NO_CAST_TO_ASCII
OTHER_FILES += Qt4ProjectManager.pluginspec
