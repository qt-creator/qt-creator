add_qtc_plugin(ProjectExplorer
  DEPENDS Qt::Qml
  PLUGIN_DEPENDS Core TextEditor
  PLUGIN_TEST_DEPENDS GenericProjectManager
  SOURCES
    abi.cpp abi.h
    abiwidget.cpp abiwidget.h
    abstractprocessstep.cpp abstractprocessstep.h
    addrunconfigdialog.cpp addrunconfigdialog.h
    allprojectsfilter.cpp allprojectsfilter.h
    allprojectsfind.cpp allprojectsfind.h
    appoutputpane.cpp appoutputpane.h
    baseprojectwizarddialog.cpp baseprojectwizarddialog.h
    buildaspects.cpp buildaspects.h
    buildpropertiessettings.cpp buildpropertiessettings.h
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
    codestylesettingspropertiespage.cpp codestylesettingspropertiespage.h
    compileoutputwindow.cpp compileoutputwindow.h
    configtaskhandler.cpp configtaskhandler.h
    copystep.cpp copystep.h
    copytaskhandler.cpp copytaskhandler.h
    currentprojectfilter.cpp currentprojectfilter.h
    currentprojectfind.cpp currentprojectfind.h
    customexecutablerunconfiguration.cpp customexecutablerunconfiguration.h
    customparser.cpp customparser.h
    customparserconfigdialog.cpp customparserconfigdialog.h
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
    devicesupport/desktopprocesssignaloperation.cpp devicesupport/desktopprocesssignaloperation.h
    devicesupport/devicecheckbuildstep.cpp devicesupport/devicecheckbuildstep.h
    devicesupport/devicefactoryselectiondialog.cpp devicesupport/devicefactoryselectiondialog.h
    devicesupport/devicemanager.cpp devicesupport/devicemanager.h
    devicesupport/devicemanagermodel.cpp devicesupport/devicemanagermodel.h
    devicesupport/deviceprocessesdialog.cpp devicesupport/deviceprocessesdialog.h
    devicesupport/devicesettingspage.cpp devicesupport/devicesettingspage.h
    devicesupport/devicetestdialog.cpp devicesupport/devicetestdialog.h
    devicesupport/deviceusedportsgatherer.cpp devicesupport/deviceusedportsgatherer.h
    devicesupport/filetransfer.cpp devicesupport/filetransfer.h
    devicesupport/filetransferinterface.h
    devicesupport/idevice.cpp devicesupport/idevice.h
    devicesupport/idevicefactory.cpp devicesupport/idevicefactory.h
    devicesupport/idevicefwd.h
    devicesupport/idevicewidget.h
    devicesupport/processlist.cpp devicesupport/processlist.h
    devicesupport/sshparameters.cpp devicesupport/sshparameters.h
    devicesupport/sshsettings.cpp devicesupport/sshsettings.h
    devicesupport/sshsettingspage.cpp devicesupport/sshsettingspage.h
    editorconfiguration.cpp editorconfiguration.h
    editorsettingspropertiespage.cpp editorsettingspropertiespage.h
    environmentaspect.cpp environmentaspect.h
    environmentaspectwidget.cpp environmentaspectwidget.h
    environmentwidget.cpp environmentwidget.h
    expanddata.cpp expanddata.h
    extraabi.cpp extraabi.h
    extracompiler.cpp extracompiler.h
    fileinsessionfinder.cpp fileinsessionfinder.h
    filesinallprojectsfind.h
    filesinallprojectsfind.cpp
    filterkitaspectsdialog.cpp filterkitaspectsdialog.h
    gccparser.cpp gccparser.h
    gcctoolchain.cpp gcctoolchain.h
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
    jsonwizard/wizarddebug.h
    jsonwizard/jsonwizardscannergenerator.cpp jsonwizard/jsonwizardscannergenerator.h
    kit.cpp kit.h
    kitaspects.cpp kitaspects.h
    kitchooser.cpp kitchooser.h
    kitfeatureprovider.h
    kitmanager.cpp kitmanager.h
    kitmanagerconfigwidget.cpp kitmanagerconfigwidget.h
    kitoptionspage.cpp kitoptionspage.h
    ldparser.cpp ldparser.h
    lldparser.cpp lldparser.h
    linuxiccparser.cpp linuxiccparser.h
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
    projectcommentssettings.cpp projectcommentssettings.h
    projectconfiguration.cpp projectconfiguration.h
    projectconfigurationmodel.cpp projectconfigurationmodel.h
    projectexplorer.cpp projectexplorer.h
    projectexplorer.qrc
    projectexplorer_export.h
    projectexplorerconstants.cpp
    projectexplorerconstants.h
    projectexplorericons.cpp projectexplorericons.h
    projectexplorersettings.cpp projectexplorersettings.h
    projectexplorertr.h
    projectfilewizardextension.cpp projectfilewizardextension.h
    projectimporter.cpp projectimporter.h
    projectmacro.cpp projectmacro.h
    projectmanager.cpp projectmanager.h
    projectmodels.cpp projectmodels.h
    projectnodes.cpp projectnodes.h
    projectpanelfactory.cpp projectpanelfactory.h
    projectsettingswidget.cpp projectsettingswidget.h
    projecttree.cpp projecttree.h
    projecttreewidget.cpp projecttreewidget.h
    projectwelcomepage.cpp projectwelcomepage.h
    projectwindow.cpp projectwindow.h
    projectwizardpage.cpp projectwizardpage.h
    rawprojectpart.cpp rawprojectpart.h
    removetaskhandler.cpp removetaskhandler.h
    runconfiguration.cpp runconfiguration.h
    runconfigurationaspects.cpp runconfigurationaspects.h
    runcontrol.cpp runcontrol.h
    runsettingspropertiespage.cpp runsettingspropertiespage.h
    sanitizerparser.cpp sanitizerparser.h
    selectablefilesmodel.cpp selectablefilesmodel.h
    showineditortaskhandler.cpp showineditortaskhandler.h
    showoutputtaskhandler.cpp showoutputtaskhandler.h
    simpleprojectwizard.cpp simpleprojectwizard.h
    target.cpp target.h
    targetsettingspanel.cpp targetsettingspanel.h
    targetsetuppage.cpp targetsetuppage.h
    targetsetupwidget.cpp targetsetupwidget.h
    task.cpp task.h
    taskfile.cpp taskfile.h
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
    windebuginterface.cpp windebuginterface.h
    xcodebuildparser.cpp xcodebuildparser.h
)

if (TARGET clangd)
  set(CLANG_BINDIR "$<TARGET_FILE_DIR:clangd>")
endif()
extend_qtc_plugin(ProjectExplorer
  DEFINES "CLANG_BINDIR=\"${CLANG_BINDIR}\""
)

extend_qtc_plugin(ProjectExplorer
  CONDITION WIN32
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
    outputparser_test.cpp outputparser_test.h
)
extend_qtc_plugin(ProjectExplorer
  CONDITION WITH_TESTS
  SOURCES
    jsonwizard/jsonwizard_test.cpp
  SOURCES_PROPERTIES HEADER_FILE_ONLY ON
)

file(GLOB_RECURSE test_resources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} testdata/*)
qtc_add_resources(ProjectExplorer "testdata"
  CONDITION WITH_TESTS
  PREFIX "/projectexplorer"
  BASE "."
  FILES ${test_resources}
)
