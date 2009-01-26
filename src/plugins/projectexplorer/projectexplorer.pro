TEMPLATE = lib
TARGET = ProjectExplorer
QT += xml \
    script
include(../../qworkbenchplugin.pri)
include(projectexplorer_dependencies.pri)
include(../../shared/scriptwrapper/scriptwrapper.pri)
HEADERS += projectexplorer.h \
    projectexplorer_export.h \
    projectwindow.h \
    buildmanager.h \
    compileoutputwindow.h \
    taskwindow.h \
    outputwindow.h \
    persistentsettings.h \
    projectfilewizardextension.h \
    session.h \
    dependenciespanel.h \
    allprojectsfilter.h \
    buildparserinterface.h \
    projectexplorerconstants.h \
    project.h \
    pluginfilefactory.h \
    iprojectmanager.h \
    currentprojectfilter.h \
    scriptwrappers.h \
    allprojectsfind.h \
    buildstep.h \
    buildconfiguration.h \
    environment.h \
    iprojectproperties.h \
    buildsettingspropertiespage.h \
    environmenteditmodel.h \
    processstep.h \
    abstractprocessstep.h \
    editorconfiguration.h \
    editorsettingspropertiespage.h \
    runconfiguration.h \
    applicationlauncher.h \
    consoleprocess.h \
    abstractprocess.h \
    applicationrunconfiguration.h \
    runsettingspropertiespage.h \
    projecttreewidget.h \
    foldernavigationwidget.h \
    customexecutablerunconfiguration.h \
    buildprogress.h \
    projectnodes.h \
    sessiondialog.h \
    projectwizardpage.h \
    buildstepspage.h \
    removefiledialog.h \
    nodesvisitor.h \
    projectmodels.h \
    currentprojectfind.h
SOURCES += projectexplorer.cpp \
    projectwindow.cpp \
    buildmanager.cpp \
    compileoutputwindow.cpp \
    taskwindow.cpp \
    outputwindow.cpp \
    persistentsettings.cpp \
    projectfilewizardextension.cpp \
    session.cpp \
    dependenciespanel.cpp \
    allprojectsfilter.cpp \
    currentprojectfilter.cpp \
    scriptwrappers.cpp \
    allprojectsfind.cpp \
    project.cpp \
    pluginfilefactory.cpp \
    buildstep.cpp \
    buildconfiguration.cpp \
    buildparserinterface.cpp \
    environment.cpp \
    buildsettingspropertiespage.cpp \
    environmenteditmodel.cpp \
    processstep.cpp \
    abstractprocessstep.cpp \
    editorconfiguration.cpp \
    editorsettingspropertiespage.cpp \
    runconfiguration.cpp \
    applicationrunconfiguration.cpp \
    runsettingspropertiespage.cpp \
    projecttreewidget.cpp \
    foldernavigationwidget.cpp \
    customexecutablerunconfiguration.cpp \
    buildprogress.cpp \
    projectnodes.cpp \
    sessiondialog.cpp \
    projectwizardpage.cpp \
    buildstepspage.cpp \
    removefiledialog.cpp \
    nodesvisitor.cpp \
    projectmodels.cpp \
    currentprojectfind.cpp
FORMS += dependenciespanel.ui \
    buildsettingspropertiespage.ui \
    processstep.ui \
    editorsettingspropertiespage.ui \
    runsettingspropertiespage.ui \
    sessiondialog.ui \
    projectwizardpage.ui \
    buildstepspage.ui \
    removefiledialog.ui
win32 { 
    SOURCES += consoleprocess_win.cpp \
        applicationlauncher_win.cpp \
        winguiprocess.cpp
    HEADERS += winguiprocess.h
}
else:unix:SOURCES += consoleprocess_unix.cpp \
    applicationlauncher_x11.cpp
RESOURCES += projectexplorer.qrc
DEFINES += PROJECTEXPLORER_LIBRARY
