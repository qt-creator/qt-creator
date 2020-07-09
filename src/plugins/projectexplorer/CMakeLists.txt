add_qtc_plugin(ProjectExplorer
  DEPENDS QtcSsh Qt5::Qml
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    abi.cpp abi.h
    abiwidget.cpp abiwidget.h
    abstractprocessstep.cpp abstractprocessstep.h
    addrunconfigdialog.cpp addrunconfigdialog.h
    allprojectsfilter.cpp allprojectsfilter.h
    allprojectsfind.cpp allprojectsfind.h
    applicationlauncher.cpp applicationlauncher.h
    appoutputpane.cpp appoutputpane.h
    baseprojectwizarddialog.cpp baseprojectwizarddialog.h
    buildaspects.cpp buildaspects.h
    buildpropertiessettings.h
    buildpropertiessettingspage.cpp buildpropertiessettingspage.h
    buildconfiguration.cpp buildconfiguration.h
    buildinfo.cpp buildinfo.h
    buildmanager.cpp buildmanager.h
    buildprogress.cpp buildprogress.h
    buildsettingspropertiespage.cpp buildsettingspropertiespage.h
    buildstep.cpp buildstep.h
    buildsteplist.cpp buildsteplist.h
    buildstepspage.cpp buildstepspage.h
    buildsystem.cpp buildsystem.h
    buildtargetinfo.h
    buildtargettype.h
    clangparser.cpp clangparser.h
    codestylesettingspropertiespage.cpp codestylesettingspropertiespage.h codestylesettingspropertiespage.ui
    compileoutputwindow.cpp compileoutputwindow.h
    configtaskhandler.cpp configtaskhandler.h
    copytaskhandler.cpp copytaskhandler.h
    currentprojectfilter.cpp currentprojectfilter.h
    currentprojectfind.cpp currentprojectfind.h
    customexecutablerunconfiguration.cpp customexecutablerunconfiguration.h
    customparser.cpp customparser.h
    customparserconfigdialog.cpp customparserconfigdialog.h customparserconfigdialog.ui
    customparserssettingspage.cpp customparserssettingspage.h
    customtoolchain.cpp customtoolchain.h
    customwizard/customwizard.cpp customwizard/customwizard.h
    customwizard/customwizardpage.cpp customwizard/customwizardpage.h
    customwizard/customwizardparameters.cpp customwizard/customwizardparameters.h
    customwizard/customwizardscriptgenerator.cpp customwizard/customwizardscriptgenerator.h
    dependenciespanel.cpp dependenciespanel.h
    deployablefile.cpp deployablefile.h
    deployconfiguration.cpp deployconfiguration.h
    deploymentdata.cpp deploymentdata.h
    deploymentdataview.cpp deploymentdataview.h
    desktoprunconfiguration.cpp desktoprunconfiguration.h
    devicesupport/desktopdevice.cpp devicesupport/desktopdevice.h
    devicesupport/desktopdevicefactory.cpp devicesupport/desktopdevicefactory.h
    devicesupport/desktopdeviceprocess.cpp devicesupport/desktopdeviceprocess.h
    devicesupport/desktopprocesssignaloperation.cpp devicesupport/desktopprocesssignaloperation.h
    devicesupport/devicecheckbuildstep.cpp devicesupport/devicecheckbuildstep.h
    devicesupport/devicefactoryselectiondialog.cpp devicesupport/devicefactoryselectiondialog.h devicesupport/devicefactoryselectiondialog.ui
    devicesupport/devicemanager.cpp devicesupport/devicemanager.h
    devicesupport/devicemanagermodel.cpp devicesupport/devicemanagermodel.h
    devicesupport/deviceprocess.cpp devicesupport/deviceprocess.h
    devicesupport/deviceprocessesdialog.cpp devicesupport/deviceprocessesdialog.h
    devicesupport/deviceprocesslist.cpp devicesupport/deviceprocesslist.h
    devicesupport/devicesettingspage.cpp devicesupport/devicesettingspage.h
    devicesupport/devicesettingswidget.cpp devicesupport/devicesettingswidget.h devicesupport/devicesettingswidget.ui
    devicesupport/devicetestdialog.cpp devicesupport/devicetestdialog.h devicesupport/devicetestdialog.ui
    devicesupport/deviceusedportsgatherer.cpp devicesupport/deviceusedportsgatherer.h
    devicesupport/idevice.cpp devicesupport/idevice.h
    devicesupport/idevicefactory.cpp devicesupport/idevicefactory.h
    devicesupport/idevicewidget.h
    devicesupport/localprocesslist.cpp devicesupport/localprocesslist.h
    devicesupport/sshdeviceprocess.cpp devicesupport/sshdeviceprocess.h
    devicesupport/sshdeviceprocess.cpp devicesupport/sshdeviceprocess.h
    devicesupport/sshdeviceprocesslist.cpp devicesupport/sshdeviceprocesslist.h
    devicesupport/sshsettingspage.cpp devicesupport/sshsettingspage.h
    editorconfiguration.cpp editorconfiguration.h
    editorsettingspropertiespage.cpp editorsettingspropertiespage.h editorsettingspropertiespage.ui
    environmentaspect.cpp environmentaspect.h
    environmentaspectwidget.cpp environmentaspectwidget.h
    environmentwidget.cpp environmentwidget.h
    expanddata.cpp expanddata.h
    extraabi.cpp extraabi.h
    extracompiler.cpp extracompiler.h
    fileinsessionfinder.cpp fileinsessionfinder.h
    filterkitaspectsdialog.cpp filterkitaspectsdialog.h
    foldernavigationwidget.cpp foldernavigationwidget.h
    gccparser.cpp gccparser.h
    gcctoolchain.cpp gcctoolchain.h
    gcctoolchainfactories.h
    gnumakeparser.cpp gnumakeparser.h
    headerpath.h
    importwidget.cpp importwidget.h
    ioutputparser.cpp ioutputparser.h
    ipotentialkit.h
    itaskhandler.h
    jsonwizard/jsonfieldpage.cpp jsonwizard/jsonfieldpage.h jsonwizard/jsonfieldpage_p.h
    jsonwizard/jsonfilepage.cpp jsonwizard/jsonfilepage.h
    jsonwizard/jsonkitspage.cpp jsonwizard/jsonkitspage.h
    jsonwizard/jsonprojectpage.cpp jsonwizard/jsonprojectpage.h
    jsonwizard/jsonsummarypage.cpp jsonwizard/jsonsummarypage.h
    jsonwizard/jsonwizard.cpp jsonwizard/jsonwizard.h
    jsonwizard/jsonwizardfactory.cpp jsonwizard/jsonwizardfactory.h
    jsonwizard/jsonwizardfilegenerator.cpp jsonwizard/jsonwizardfilegenerator.h
    jsonwizard/jsonwizardgeneratorfactory.cpp jsonwizard/jsonwizardgeneratorfactory.h
    jsonwizard/jsonwizardpagefactory.cpp jsonwizard/jsonwizardpagefactory.h
    jsonwizard/jsonwizardpagefactory_p.cpp
    jsonwizard/jsonwizardpagefactory_p.h
    jsonwizard/jsonwizardscannergenerator.cpp jsonwizard/jsonwizardscannergenerator.h
    kit.cpp kit.h
    kitchooser.cpp kitchooser.h
    kitfeatureprovider.h
    kitinformation.cpp kitinformation.h
    kitmanager.cpp kitmanager.h
    kitmanagerconfigwidget.cpp kitmanagerconfigwidget.h
    kitmodel.cpp kitmodel.h
    kitoptionspage.cpp kitoptionspage.h
    ldparser.cpp ldparser.h
    lldparser.cpp lldparser.h
    linuxiccparser.cpp linuxiccparser.h
    localenvironmentaspect.cpp localenvironmentaspect.h
    makestep.cpp makestep.h
    miniprojecttargetselector.cpp miniprojecttargetselector.h
    msvcparser.cpp msvcparser.h
    msvctoolchain.cpp msvctoolchain.h
    namedwidget.cpp namedwidget.h
    osparser.cpp osparser.h
    panelswidget.cpp panelswidget.h
    parseissuesdialog.cpp parseissuesdialog.h
    processparameters.cpp processparameters.h
    processstep.cpp processstep.h
    project.cpp project.h
    projectconfiguration.cpp projectconfiguration.h
    projectconfigurationaspects.cpp projectconfigurationaspects.h
    projectconfigurationmodel.cpp projectconfigurationmodel.h
    projectexplorer.cpp projectexplorer.h
    projectexplorer.qrc
    projectexplorer_export.h
    projectexplorerconstants.h
    projectexplorericons.cpp projectexplorericons.h
    projectexplorersettings.h
    projectexplorersettingspage.cpp projectexplorersettingspage.h projectexplorersettingspage.ui
    projectfilewizardextension.cpp projectfilewizardextension.h
    projectimporter.cpp projectimporter.h
    projectmacro.cpp projectmacro.h
    projectmacroexpander.cpp projectmacroexpander.h
    projectmanager.h
    projectmodels.cpp projectmodels.h
    projectnodes.cpp projectnodes.h
    projectpanelfactory.cpp projectpanelfactory.h
    projecttree.cpp projecttree.h
    projecttreewidget.cpp projecttreewidget.h
    projectwelcomepage.cpp projectwelcomepage.h
    projectwindow.cpp projectwindow.h
    projectwizardpage.cpp projectwizardpage.h projectwizardpage.ui
    rawprojectpart.cpp rawprojectpart.h
    removetaskhandler.cpp removetaskhandler.h
    runconfiguration.cpp runconfiguration.h
    runconfigurationaspects.cpp runconfigurationaspects.h
    runcontrol.cpp runcontrol.h
    runsettingspropertiespage.cpp runsettingspropertiespage.h
    selectablefilesmodel.cpp selectablefilesmodel.h
    session.cpp session.h
    sessiondialog.cpp sessiondialog.h sessiondialog.ui
    sessionmodel.cpp sessionmodel.h
    sessionview.cpp sessionview.h
    showineditortaskhandler.cpp showineditortaskhandler.h
    showoutputtaskhandler.cpp showoutputtaskhandler.h
    simpleprojectwizard.cpp simpleprojectwizard.h
    target.cpp target.h
    targetsettingspanel.cpp targetsettingspanel.h
    targetsetuppage.cpp targetsetuppage.h
    targetsetupwidget.cpp targetsetupwidget.h
    task.cpp task.h
    taskhub.cpp taskhub.h
    taskmodel.cpp taskmodel.h
    taskwindow.cpp taskwindow.h
    toolchain.cpp toolchain.h
    toolchaincache.h
    toolchainconfigwidget.cpp toolchainconfigwidget.h
    toolchainmanager.cpp toolchainmanager.h
    toolchainoptionspage.cpp toolchainoptionspage.h
    toolchainsettingsaccessor.cpp toolchainsettingsaccessor.h
    treescanner.cpp treescanner.h
    userfileaccessor.cpp userfileaccessor.h
    vcsannotatetaskhandler.cpp vcsannotatetaskhandler.h
    waitforstopdialog.cpp waitforstopdialog.h
    xcodebuildparser.cpp xcodebuildparser.h
)

if (TARGET libclang)
  set(CLANG_BINDIR "$<TARGET_FILE_DIR:libclang>")
endif()
extend_qtc_plugin(ProjectExplorer
  DEFINES "CLANG_BINDIR=\"${CLANG_BINDIR}\""
)

extend_qtc_plugin(ProjectExplorer
  CONDITION WIN32
  SOURCES windebuginterface.cpp windebuginterface.h
  DEFINES UNICODE _UNICODE
)

extend_qtc_plugin(ProjectExplorer
  CONDITION journald
  DEPENDS systemd
  SOURCES journaldwatcher.cpp journaldwatcher.h
  DEFINES WITH_JOURNALD
)

extend_qtc_plugin(ProjectExplorer
  CONDITION WITH_TESTS
  SOURCES
    jsonwizard/jsonwizard_test.cpp
    outputparser_test.cpp outputparser_test.h
)
qtc_plugin_enabled(_projectexplorer_enabled ProjectExplorer)
if (WITH_TESTS AND _projectexplorer_enabled)
  set_source_files_properties(jsonwizard/jsonwizard_test.cpp
    PROPERTIES HEADER_FILE_ONLY ON
  )
endif()

