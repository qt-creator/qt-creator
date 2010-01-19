TEMPLATE = lib
TARGET = ProjectExplorer
QT += xml \
    script
include(../../qtcreatorplugin.pri)
include(projectexplorer_dependencies.pri)
include(../../shared/scriptwrapper/scriptwrapper.pri)
include(../../libs/utils/utils.pri)
INCLUDEPATH += $$PWD/../../libs/utils
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
    ioutputparser.h \
    projectconfiguration.h \
    gnumakeparser.h \
    projectexplorerconstants.h \
    projectexplorersettings.h \
    corelistenercheckingforrunningbuild.h \
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
    currentprojectfind.h \
    toolchain.h \
    userfileaccessor.h \
    cesdkhandler.h \
    gccparser.h \
    msvcparser.h \
    filewatcher.h \
    debugginghelper.h \
    projectexplorersettingspage.h \
    projectwelcomepage.h \
    projectwelcomepagewidget.h \
    baseprojectwizarddialog.h \
    miniprojecttargetselector.h \
    targetselector.h \
    targetsettingswidget.h \
    doubletabwidget.h
SOURCES += projectexplorer.cpp \
    projectwindow.cpp \
    buildmanager.cpp \
    compileoutputwindow.cpp \
    ioutputparser.cpp \
    projectconfiguration.cpp \
    gnumakeparser.cpp \
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
    currentprojectfind.cpp \
    toolchain.cpp \
    cesdkhandler.cpp \
    userfileaccessor.cpp \
    gccparser.cpp \
    msvcparser.cpp \
    filewatcher.cpp \
    debugginghelper.cpp \
    projectexplorersettingspage.cpp \
    projectwelcomepage.cpp \
    projectwelcomepagewidget.cpp \
    corelistenercheckingforrunningbuild.cpp \
    baseprojectwizarddialog.cpp \
    miniprojecttargetselector.cpp \
    targetselector.cpp \
    targetsettingswidget.cpp \
    doubletabwidget.cpp
FORMS += processstep.ui \
    editorsettingspropertiespage.ui \
    runsettingspropertiespage.ui \
    sessiondialog.ui \
    projectwizardpage.ui \
    removefiledialog.ui \
    projectexplorersettingspage.ui \
    projectwelcomepagewidget.ui \
    targetsettingswidget.ui \
    doubletabwidget.ui

win32 {
    SOURCES += applicationlauncher_win.cpp \
        winguiprocess.cpp
    HEADERS += winguiprocess.h
}
else:macx {
    SOURCES += applicationlauncher_x11.cpp
    LIBS += -framework \
        Carbon
}
else:unix {
    SOURCES += applicationlauncher_x11.cpp
}

RESOURCES += projectexplorer.qrc
DEFINES += PROJECTEXPLORER_LIBRARY
OTHER_FILES += ProjectExplorer.pluginspec
