TEMPLATE = lib
TARGET = Qt4ProjectManager
QT += network
include(../../qworkbenchplugin.pri)
include(qt4projectmanager_dependencies.pri)
HEADERS = qt4projectmanagerplugin.h \
    qt4projectmanager.h \
    qtversionmanager.h \
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
    wizards/modulespage.h \
    wizards/filespage.h \
    wizards/qtwizard.h \
    qt4projectmanagerconstants.h \
    makestep.h \
    qmakestep.h \
    qmakebuildstepfactory.h \
    gccparser.h \
    msvcparser.h \
    buildparserfactory.h \
    deployhelper.h \
    msvcenvironment.h \
    cesdkhandler.h \
    embeddedpropertiespage.h \
    qt4runconfiguration.h \
    speinfo.h \
    headerpath.h \
    gccpreprocessor.h \
    qt4buildconfigwidget.h \
    qt4buildenvironmentwidget.h \
    projectloadwizard.h \
    directorywatcher.h \
    gdbmacrosbuildstep.h
SOURCES = qt4projectmanagerplugin.cpp \
    qt4projectmanager.cpp \
    qtversionmanager.cpp \
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
    wizards/modulespage.cpp \
    wizards/filespage.cpp \
    wizards/qtwizard.cpp \
    makestep.cpp \
    qmakestep.cpp \
    qmakebuildstepfactory.cpp \
    gccparser.cpp \
    msvcparser.cpp \
    buildparserfactory.cpp \
    deployhelper.cpp \
    msvcenvironment.cpp \
    cesdkhandler.cpp \
    embeddedpropertiespage.cpp \
    qt4runconfiguration.cpp \
    speinfo.cpp \
    gccpreprocessor.cpp \
    qt4buildconfigwidget.cpp \
    qt4buildenvironmentwidget.cpp \
    projectloadwizard.cpp \
    directorywatcher.cpp \
    gdbmacrosbuildstep.cpp
FORMS = qtversionmanager.ui \
    envvariablespage.ui \
    enveditdialog.ui \
    proeditorcontainer.ui \
    makestep.ui \
    qmakestep.ui \
    qt4buildconfigwidget.ui \
    embeddedpropertiespage.ui \
    qt4buildenvironmentwidget.ui
RESOURCES = qt4projectmanager.qrc \
    wizards/wizards.qrc
include(../../shared/proparser/proparser.pri)
DEFINES += QT_NO_CAST_TO_ASCII
