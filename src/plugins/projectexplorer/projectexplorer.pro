QT += qml

include(../../qtcreatorplugin.pri)
include(customwizard/customwizard.pri)
include(jsonwizard/jsonwizard.pri)

include(../../shared/clang/clang_installation.pri)
include(../../shared/clang/clang_defines.pri)

HEADERS += projectexplorer.h \
    abi.h \
    abiwidget.h \
    addrunconfigdialog.h \
    buildaspects.h \
    buildinfo.h \
    buildpropertiessettings.h \
    buildpropertiessettingspage.h \
    buildsystem.h \
    buildtargettype.h \
    clangparser.h \
    configtaskhandler.h \
    customparserssettingspage.h \
    desktoprunconfiguration.h \
    environmentaspect.h \
    environmentaspectwidget.h \
    extraabi.h \
    fileinsessionfinder.h \
    filterkitaspectsdialog.h \
    gcctoolchain.h \
    importwidget.h \
    userfileaccessor.h \
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
    kitinformation.h \
    kitfeatureprovider.h \
    kitmanager.h \
    kitmanagerconfigwidget.h \
    kitmodel.h \
    kitoptionspage.h \
    projectconfigurationmodel.h \
    buildmanager.h \
    buildsteplist.h \
    compileoutputwindow.h \
    deployconfiguration.h \
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
    runcontrol.h \
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
    toolchaincache.h \
    toolchainconfigwidget.h \
    toolchainmanager.h \
    toolchainoptionspage.h \
    toolchainsettingsaccessor.h \
    gccparser.h \
    projectexplorersettingspage.h \
    baseprojectwizarddialog.h \
    miniprojecttargetselector.h \
    ldparser.h \
    lldparser.h \
    linuxiccparser.h \
    runconfigurationaspects.h \
    processparameters.h \
    abstractprocessstep.h \
    taskhub.h \
    headerpath.h \
    gcctoolchainfactories.h \
    appoutputpane.h \
    codestylesettingspropertiespage.h \
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
    devicesupport/sshsettingspage.h \
    devicesupport/desktopprocesssignaloperation.h \
    deploymentdata.h \
    deploymentdataview.h \
    buildtargetinfo.h \
    customtoolchain.h \
    projectmacroexpander.h \
    customparser.h \
    customparserconfigdialog.h \
    ipotentialkit.h \
    msvcparser.h \
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
    extracompiler.h \
    customexecutablerunconfiguration.h \
    projectmacro.h \
    makestep.h \
    parseissuesdialog.h \
    projectconfigurationaspects.h \
    treescanner.h \
    rawprojectpart.h \
    simpleprojectwizard.h

SOURCES += projectexplorer.cpp \
    abi.cpp \
    abiwidget.cpp \
    addrunconfigdialog.cpp \
    buildaspects.cpp \
    buildinfo.cpp \
    buildpropertiessettingspage.cpp \
    buildsystem.cpp \
    clangparser.cpp \
    customparserssettingspage.cpp \
    configtaskhandler.cpp \
    desktoprunconfiguration.cpp \
    environmentaspect.cpp \
    environmentaspectwidget.cpp \
    extraabi.cpp \
    fileinsessionfinder.cpp \
    filterkitaspectsdialog.cpp \
    gcctoolchain.cpp \
    importwidget.cpp \
    projectconfigurationmodel.cpp \
    userfileaccessor.cpp \
    localenvironmentaspect.cpp \
    osparser.cpp \
    projectimporter.cpp \
    projectwindow.cpp \
    removetaskhandler.cpp \
    targetsetuppage.cpp \
    targetsetupwidget.cpp \
    kit.cpp \
    kitchooser.cpp \
    kitinformation.cpp \
    kitmanager.cpp \
    kitmanagerconfigwidget.cpp \
    kitmodel.cpp \
    kitoptionspage.cpp \
    buildmanager.cpp \
    buildsteplist.cpp \
    compileoutputwindow.cpp \
    deployconfiguration.cpp \
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
    runcontrol.cpp \
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
    toolchainsettingsaccessor.cpp \
    gccparser.cpp \
    projectexplorersettingspage.cpp \
    baseprojectwizarddialog.cpp \
    miniprojecttargetselector.cpp \
    ldparser.cpp \
    lldparser.cpp \
    linuxiccparser.cpp \
    runconfigurationaspects.cpp \
    taskhub.cpp \
    processparameters.cpp \
    appoutputpane.cpp \
    codestylesettingspropertiespage.cpp \
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
    devicesupport/sshsettingspage.cpp \
    devicesupport/desktopprocesssignaloperation.cpp \
    deployablefile.cpp \
    deploymentdata.cpp \
    deploymentdataview.cpp \
    customtoolchain.cpp \
    projectmacroexpander.cpp \
    customparser.cpp \
    customparserconfigdialog.cpp \
    msvcparser.cpp \
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
    customexecutablerunconfiguration.cpp \
    projectmacro.cpp \
    makestep.cpp \
    parseissuesdialog.cpp \
    projectconfigurationaspects.cpp \
    treescanner.cpp \
    rawprojectpart.cpp \
    simpleprojectwizard.cpp

FORMS += \
    editorsettingspropertiespage.ui \
    sessiondialog.ui \
    projectwizardpage.ui \
    projectexplorersettingspage.ui \
    codestylesettingspropertiespage.ui \
    devicesupport/devicefactoryselectiondialog.ui \
    devicesupport/devicesettingswidget.ui \
    devicesupport/devicetestdialog.ui \
    customparserconfigdialog.ui \

WINSOURCES += \
    windebuginterface.cpp \
    msvctoolchain.cpp \

WINHEADERS += \
    windebuginterface.h \
    msvctoolchain.h \

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

RESOURCES += projectexplorer.qrc

DEFINES += PROJECTEXPLORER_LIBRARY

!isEmpty(PROJECT_USER_FILE_EXTENSION) {
    DEFINES += PROJECT_USER_FILE_EXTENSION=$${PROJECT_USER_FILE_EXTENSION}
}
