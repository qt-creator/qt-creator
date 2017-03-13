QT += qml

include(../../qtcreatorplugin.pri)
include(customwizard/customwizard.pri)
include(jsonwizard/jsonwizard.pri)
HEADERS += projectexplorer.h \
    abi.h \
    abiwidget.h \
    ansifilterparser.h \
    buildinfo.h \
    clangparser.h \
    configtaskhandler.h \
    environmentaspect.h \
    environmentaspectwidget.h \
    gcctoolchain.h \
    importwidget.h \
    runnables.h \
    localenvironmentaspect.h \
    osparser.h \
    projectexplorer_export.h \
    projectimporter.h \
    projectwindow.h \
    removetaskhandler.h \
    targetsetuppage.h \
    targetsetupwidget.h \
    kit.h \
    kitchooser.h \
    kitconfigwidget.h \
    kitinformation.h \
    kitinformationconfigwidget.h \
    kitfeatureprovider.h \
    kitmanager.h \
    kitmanagerconfigwidget.h \
    kitmodel.h \
    kitoptionspage.h \
    buildmanager.h \
    buildsteplist.h \
    compileoutputwindow.h \
    deployconfiguration.h \
    deployconfigurationmodel.h \
    namedwidget.h \
    target.h \
    targetsettingspanel.h \
    task.h \
    itaskhandler.h \
    copytaskhandler.h \
    showineditortaskhandler.h \
    showoutputtaskhandler.h \
    vcsannotatetaskhandler.h \
    taskwindow.h \
    taskmodel.h \
    projectfilewizardextension.h \
    session.h \
    dependenciespanel.h \
    allprojectsfilter.h \
    ioutputparser.h \
    projectconfiguration.h \
    gnumakeparser.h \
    projectexplorerconstants.h \
    projectexplorersettings.h \
    project.h \
    projectmanager.h \
    currentprojectfilter.h \
    allprojectsfind.h \
    buildstep.h \
    buildconfiguration.h \
    buildsettingspropertiespage.h \
    environmentwidget.h \
    processstep.h \
    editorconfiguration.h \
    editorsettingspropertiespage.h \
    runconfiguration.h \
    applicationlauncher.h \
    runsettingspropertiespage.h \
    projecttreewidget.h \
    foldernavigationwidget.h \
    buildprogress.h \
    projectnodes.h \
    sessiondialog.h \
    sessionview.h \
    projectwizardpage.h \
    buildstepspage.h \
    projectmodels.h \
    currentprojectfind.h \
    toolchain.h \
    toolchainconfigwidget.h \
    toolchainmanager.h \
    toolchainoptionspage.h \
    cesdkhandler.h \
    gccparser.h \
    projectexplorersettingspage.h \
    baseprojectwizarddialog.h \
    miniprojecttargetselector.h \
    buildenvironmentwidget.h \
    ldparser.h \
    linuxiccparser.h \
    runconfigurationaspects.h \
    runconfigurationmodel.h \
    buildconfigurationmodel.h \
    processparameters.h \
    abstractprocessstep.h \
    taskhub.h \
    localapplicationruncontrol.h \
    headerpath.h \
    gcctoolchainfactories.h \
    appoutputpane.h \
    codestylesettingspropertiespage.h \
    settingsaccessor.h \
    deployablefile.h \
    devicesupport/idevice.h \
    devicesupport/desktopdevice.h \
    devicesupport/desktopdevicefactory.h \
    devicesupport/idevicewidget.h \
    devicesupport/idevicefactory.h \
    devicesupport/desktopdeviceprocess.h \
    devicesupport/devicecheckbuildstep.h \
    devicesupport/devicemanager.h \
    devicesupport/devicemanagermodel.h \
    devicesupport/devicefactoryselectiondialog.h \
    devicesupport/deviceprocess.h \
    devicesupport/deviceprocesslist.h \
    devicesupport/deviceprocessesdialog.h \
    devicesupport/devicesettingswidget.h \
    devicesupport/devicesettingspage.h \
    devicesupport/devicetestdialog.h \
    devicesupport/deviceusedportsgatherer.h \
    devicesupport/localprocesslist.h \
    devicesupport/sshdeviceprocess.h \
    devicesupport/sshdeviceprocesslist.h \
    devicesupport/desktopdeviceconfigurationwidget.h \
    devicesupport/desktopprocesssignaloperation.h \
    deploymentdata.h \
    deploymentdatamodel.h \
    deploymentdataview.h \
    buildtargetinfo.h \
    customtoolchain.h \
    projectmacroexpander.h \
    customparser.h \
    customparserconfigdialog.h \
    ipotentialkit.h \
    selectablefilesmodel.h \
    xcodebuildparser.h \
    panelswidget.h \
    projectwelcomepage.h \
    sessionmodel.h \
    projectpanelfactory.h \
    projecttree.h \
    expanddata.h \
    waitforstopdialog.h \
    projectexplorericons.h \
    projectexplorer_global.h \
    extracompiler.h \
    customexecutableconfigurationwidget.h \
    customexecutablerunconfiguration.h

SOURCES += projectexplorer.cpp \
    abi.cpp \
    abiwidget.cpp \
    ansifilterparser.cpp \
    buildinfo.cpp \
    clangparser.cpp \
    configtaskhandler.cpp \
    environmentaspect.cpp \
    environmentaspectwidget.cpp \
    gcctoolchain.cpp \
    importwidget.cpp \
    runnables.cpp \
    localenvironmentaspect.cpp \
    osparser.cpp \
    projectimporter.cpp \
    projectwindow.cpp \
    removetaskhandler.cpp \
    targetsetuppage.cpp \
    targetsetupwidget.cpp \
    kit.cpp \
    kitchooser.cpp \
    kitconfigwidget.cpp \
    kitinformation.cpp \
    kitinformationconfigwidget.cpp \
    kitmanager.cpp \
    kitmanagerconfigwidget.cpp \
    kitmodel.cpp \
    kitoptionspage.cpp \
    buildmanager.cpp \
    buildsteplist.cpp \
    compileoutputwindow.cpp \
    deployconfiguration.cpp \
    deployconfigurationmodel.cpp \
    namedwidget.cpp \
    target.cpp \
    targetsettingspanel.cpp \
    ioutputparser.cpp \
    projectconfiguration.cpp \
    gnumakeparser.cpp \
    task.cpp \
    copytaskhandler.cpp \
    showineditortaskhandler.cpp \
    showoutputtaskhandler.cpp \
    vcsannotatetaskhandler.cpp \
    taskwindow.cpp \
    taskmodel.cpp \
    projectfilewizardextension.cpp \
    session.cpp \
    dependenciespanel.cpp \
    allprojectsfilter.cpp \
    currentprojectfilter.cpp \
    allprojectsfind.cpp \
    project.cpp \
    buildstep.cpp \
    buildconfiguration.cpp \
    buildsettingspropertiespage.cpp \
    environmentwidget.cpp \
    processstep.cpp \
    abstractprocessstep.cpp \
    editorconfiguration.cpp \
    editorsettingspropertiespage.cpp \
    runconfiguration.cpp \
    applicationlauncher.cpp \
    runsettingspropertiespage.cpp \
    projecttreewidget.cpp \
    foldernavigationwidget.cpp \
    buildprogress.cpp \
    projectnodes.cpp \
    sessiondialog.cpp \
    sessionview.cpp \
    projectwizardpage.cpp \
    buildstepspage.cpp \
    projectmodels.cpp \
    currentprojectfind.cpp \
    toolchain.cpp \
    toolchainconfigwidget.cpp \
    toolchainmanager.cpp \
    toolchainoptionspage.cpp \
    cesdkhandler.cpp \
    gccparser.cpp \
    projectexplorersettingspage.cpp \
    baseprojectwizarddialog.cpp \
    miniprojecttargetselector.cpp \
    buildenvironmentwidget.cpp \
    ldparser.cpp \
    linuxiccparser.cpp \
    runconfigurationaspects.cpp \
    runconfigurationmodel.cpp \
    buildconfigurationmodel.cpp \
    taskhub.cpp \
    processparameters.cpp \
    localapplicationruncontrol.cpp \
    appoutputpane.cpp \
    codestylesettingspropertiespage.cpp \
    settingsaccessor.cpp \
    devicesupport/idevice.cpp \
    devicesupport/desktopdevice.cpp \
    devicesupport/desktopdevicefactory.cpp \
    devicesupport/idevicefactory.cpp \
    devicesupport/desktopdeviceprocess.cpp \
    devicesupport/devicecheckbuildstep.cpp \
    devicesupport/devicemanager.cpp \
    devicesupport/devicemanagermodel.cpp \
    devicesupport/devicefactoryselectiondialog.cpp \
    devicesupport/deviceprocess.cpp \
    devicesupport/deviceprocesslist.cpp \
    devicesupport/deviceprocessesdialog.cpp \
    devicesupport/devicesettingswidget.cpp \
    devicesupport/devicesettingspage.cpp \
    devicesupport/devicetestdialog.cpp \
    devicesupport/deviceusedportsgatherer.cpp \
    devicesupport/localprocesslist.cpp \
    devicesupport/sshdeviceprocess.cpp \
    devicesupport/sshdeviceprocesslist.cpp \
    devicesupport/desktopdeviceconfigurationwidget.cpp \
    devicesupport/desktopprocesssignaloperation.cpp \
    deployablefile.cpp \
    deploymentdatamodel.cpp \
    deploymentdataview.cpp \
    customtoolchain.cpp \
    projectmacroexpander.cpp \
    customparser.cpp \
    customparserconfigdialog.cpp \
    selectablefilesmodel.cpp \
    xcodebuildparser.cpp \
    panelswidget.cpp \
    projectwelcomepage.cpp \
    sessionmodel.cpp \
    projectpanelfactory.cpp \
    projecttree.cpp \
    expanddata.cpp \
    waitforstopdialog.cpp \
    projectexplorericons.cpp \
    extracompiler.cpp \
    customexecutableconfigurationwidget.cpp \
    customexecutablerunconfiguration.cpp

FORMS += processstep.ui \
    editorsettingspropertiespage.ui \
    sessiondialog.ui \
    projectwizardpage.ui \
    projectexplorersettingspage.ui \
    deploymentdataview.ui \
    codestylesettingspropertiespage.ui \
    devicesupport/devicefactoryselectiondialog.ui \
    devicesupport/devicesettingswidget.ui \
    devicesupport/devicetestdialog.ui \
    devicesupport/desktopdeviceconfigurationwidget.ui \
    customparserconfigdialog.ui

WINSOURCES += \
    windebuginterface.cpp \
    msvcparser.cpp \
    msvctoolchain.cpp \
    abstractmsvctoolchain.cpp \
    wincetoolchain.cpp

WINHEADERS += \
    windebuginterface.h \
    msvcparser.h \
    msvctoolchain.h \
    abstractmsvctoolchain.h \
    wincetoolchain.h

win32|equals(TEST, 1) {
    SOURCES += $$WINSOURCES
    HEADERS += $$WINHEADERS
}

equals(TEST, 1) {
    SOURCES += \
        outputparser_test.cpp
    HEADERS += \
        outputparser_test.h
}

journald {
    SOURCES += journaldwatcher.cpp
    HEADERS += journaldwatcher.h
    DEFINES += WITH_JOURNALD
    LIBS += -lsystemd
}

macx:LIBS += -framework Carbon

RESOURCES += projectexplorer.qrc

# Some way to override the architecture used in Abi:
!isEmpty($$(QTC_CPU)) {
    DEFINES += QTC_CPU=$$(QTC_CPU)
} else {
    DEFINES += QTC_CPU=X86Architecture
}

DEFINES += PROJECTEXPLORER_LIBRARY
