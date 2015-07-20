import qbs 1.0

QtcPlugin {
    name: "ProjectExplorer"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "qml"] }
    Depends { name: "Qt.quick" }
    Depends { name: "Aggregation" }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    cpp.defines: base.concat("QTC_CPU=X86Architecture")
    Properties {
        condition: qbs.targetOS.contains("osx")
        cpp.frameworks: base.concat(["Carbon"])
    }

    Group {
        name: "General"
        files: [
            "abi.cpp", "abi.h",
            "abiwidget.cpp", "abiwidget.h",
            "abstractprocessstep.cpp", "abstractprocessstep.h",
            "allprojectsfilter.cpp", "allprojectsfilter.h",
            "allprojectsfind.cpp", "allprojectsfind.h",
            "ansifilterparser.cpp", "ansifilterparser.h",
            "applicationlauncher.cpp", "applicationlauncher.h",
            "appoutputpane.cpp", "appoutputpane.h",
            "baseprojectwizarddialog.cpp", "baseprojectwizarddialog.h",
            "buildconfiguration.cpp", "buildconfiguration.h",
            "buildconfigurationmodel.cpp", "buildconfigurationmodel.h",
            "buildenvironmentwidget.cpp", "buildenvironmentwidget.h",
            "buildinfo.cpp", "buildinfo.h",
            "buildmanager.cpp", "buildmanager.h",
            "buildprogress.cpp", "buildprogress.h",
            "buildsettingspropertiespage.cpp", "buildsettingspropertiespage.h",
            "buildstep.cpp", "buildstep.h",
            "buildsteplist.cpp", "buildsteplist.h",
            "buildstepspage.cpp", "buildstepspage.h",
            "buildtargetinfo.h",
            "cesdkhandler.cpp", "cesdkhandler.h",
            "clangparser.cpp", "clangparser.h",
            "codestylesettingspropertiespage.cpp", "codestylesettingspropertiespage.h", "codestylesettingspropertiespage.ui",
            "compileoutputwindow.cpp", "compileoutputwindow.h",
            "configtaskhandler.cpp", "configtaskhandler.h",
            "copytaskhandler.cpp", "copytaskhandler.h",
            "corelistenercheckingforrunningbuild.cpp", "corelistenercheckingforrunningbuild.h",
            "currentprojectfilter.cpp", "currentprojectfilter.h",
            "currentprojectfind.cpp", "currentprojectfind.h",
            "customparser.cpp", "customparser.h",
            "customparserconfigdialog.cpp", "customparserconfigdialog.h", "customparserconfigdialog.ui",
            "customtoolchain.cpp", "customtoolchain.h",
            "dependenciespanel.cpp", "dependenciespanel.h",
            "deployablefile.cpp", "deployablefile.h",
            "deployconfiguration.cpp", "deployconfiguration.h",
            "deployconfigurationmodel.cpp", "deployconfigurationmodel.h",
            "deploymentdata.h",
            "deploymentdataview.cpp",
            "deploymentdataview.h",
            "deploymentdataview.ui",
            "deploymentdatamodel.cpp",
            "deploymentdatamodel.h",
            "doubletabwidget.cpp", "doubletabwidget.h", "doubletabwidget.ui",
            "editorconfiguration.cpp", "editorconfiguration.h",
            "editorsettingspropertiespage.cpp", "editorsettingspropertiespage.h", "editorsettingspropertiespage.ui",
            "environmentaspect.cpp", "environmentaspect.h",
            "environmentaspectwidget.cpp", "environmentaspectwidget.h",
            "environmentitemswidget.cpp", "environmentitemswidget.h",
            "environmentwidget.cpp", "environmentwidget.h",
            "expanddata.cpp", "expanddata.h",
            "foldernavigationwidget.cpp", "foldernavigationwidget.h",
            "gccparser.cpp", "gccparser.h",
            "gcctoolchain.cpp", "gcctoolchain.h",
            "gcctoolchainfactories.h",
            "gnumakeparser.cpp", "gnumakeparser.h",
            "headerpath.h",
            "importwidget.cpp", "importwidget.h",
            "ioutputparser.cpp", "ioutputparser.h",
            "ipotentialkit.cpp",
            "ipotentialkit.h",
            "iprojectmanager.h",
            "itaskhandler.h",
            "kit.cpp", "kit.h",
            "kitchooser.cpp", "kitchooser.h",
            "kitconfigwidget.cpp", "kitconfigwidget.h",
            "kitfeatureprovider.h",
            "kitinformation.cpp", "kitinformation.h",
            "kitinformationconfigwidget.cpp", "kitinformationconfigwidget.h",
            "kitmanager.cpp", "kitmanager.h",
            "kitmanagerconfigwidget.cpp", "kitmanagerconfigwidget.h",
            "kitmodel.cpp", "kitmodel.h",
            "kitoptionspage.cpp", "kitoptionspage.h",
            "ldparser.cpp", "ldparser.h",
            "linuxiccparser.cpp", "linuxiccparser.h",
            "localapplicationrunconfiguration.cpp", "localapplicationrunconfiguration.h",
            "localapplicationruncontrol.cpp", "localapplicationruncontrol.h",
            "localenvironmentaspect.cpp", "localenvironmentaspect.h",
            "metatypedeclarations.h",
            "miniprojecttargetselector.cpp", "miniprojecttargetselector.h",
            "namedwidget.cpp", "namedwidget.h",
            "nodesvisitor.cpp", "nodesvisitor.h",
            "osparser.cpp", "osparser.h",
            "panelswidget.cpp", "panelswidget.h",
            "processparameters.cpp", "processparameters.h",
            "processstep.cpp", "processstep.h", "processstep.ui",
            "project.cpp", "project.h",
            "projectconfiguration.cpp", "projectconfiguration.h",
            "projectexplorer.cpp", "projectexplorer.h",
            "projectexplorer.qrc",
            "projectexplorer_export.h",
            "projectexplorerconstants.h",
            "projectexplorersettings.h",
            "projectexplorersettingspage.cpp", "projectexplorersettingspage.h", "projectexplorersettingspage.ui",
            "projectfilewizardextension.cpp", "projectfilewizardextension.h",
            "projectimporter.cpp", "projectimporter.h",
            "projectmacroexpander.cpp", "projectmacroexpander.h",
            "projectmodels.cpp", "projectmodels.h",
            "projectnodes.cpp", "projectnodes.h",
            "projectpanelfactory.cpp", "projectpanelfactory.h",
            "projecttree.cpp",
            "projecttree.h",
            "projecttreewidget.cpp", "projecttreewidget.h",
            "projectwindow.cpp", "projectwindow.h",
            "projectwizardpage.cpp", "projectwizardpage.h", "projectwizardpage.ui",
            "propertiespanel.cpp", "propertiespanel.h",
            "removetaskhandler.cpp", "removetaskhandler.h",
            "runconfiguration.cpp", "runconfiguration.h",
            "runconfigurationaspects.cpp", "runconfigurationaspects.h",
            "runconfigurationmodel.cpp", "runconfigurationmodel.h",
            "runsettingspropertiespage.cpp", "runsettingspropertiespage.h",
            "selectablefilesmodel.cpp", "selectablefilesmodel.h",
            "session.cpp", "session.h",
            "sessiondialog.cpp", "sessiondialog.h", "sessiondialog.ui",
            "settingsaccessor.cpp", "settingsaccessor.h",
            "showineditortaskhandler.cpp", "showineditortaskhandler.h",
            "showoutputtaskhandler.cpp", "showoutputtaskhandler.h",
            "target.cpp", "target.h",
            "targetselector.cpp", "targetselector.h",
            "targetsettingspanel.cpp", "targetsettingspanel.h",
            "targetsettingswidget.cpp", "targetsettingswidget.h", "targetsettingswidget.ui",
            "targetsetuppage.cpp", "targetsetuppage.h",
            "targetsetupwidget.cpp", "targetsetupwidget.h",
            "task.cpp", "task.h",
            "taskhub.cpp", "taskhub.h",
            "taskmodel.cpp", "taskmodel.h",
            "taskwindow.cpp", "taskwindow.h",
            "toolchain.cpp", "toolchain.h",
            "toolchainconfigwidget.cpp", "toolchainconfigwidget.h",
            "toolchainmanager.cpp", "toolchainmanager.h",
            "toolchainoptionspage.cpp", "toolchainoptionspage.h",
            "unconfiguredprojectpanel.cpp", "unconfiguredprojectpanel.h",
            "vcsannotatetaskhandler.cpp", "vcsannotatetaskhandler.h",
            "xcodebuildparser.cpp", "xcodebuildparser.h"
        ]
    }

    Group {
        name: "Project Welcome Page"
        files: [
            "projectwelcomepage.cpp",
            "projectwelcomepage.h"
        ]
    }

    Group {
        name: "JsonWizard"
        prefix: "jsonwizard/"
        files: [
            "jsonfieldpage.cpp", "jsonfieldpage.h",
            "jsonfilepage.cpp", "jsonfilepage.h",
            "jsonkitspage.cpp", "jsonkitspage.h",
            "jsonprojectpage.cpp", "jsonprojectpage.h",
            "jsonsummarypage.cpp", "jsonsummarypage.h",
            "jsonwizard.cpp", "jsonwizard.h",
            "jsonwizardfactory.cpp", "jsonwizardfactory.h",
            "jsonwizardfilegenerator.cpp", "jsonwizardfilegenerator.h",
            "jsonwizardgeneratorfactory.cpp", "jsonwizardgeneratorfactory.h",
            "jsonwizardpagefactory.cpp", "jsonwizardpagefactory.h",
            "jsonwizardpagefactory_p.cpp", "jsonwizardpagefactory_p.h",
            "jsonwizardscannergenerator.cpp", "jsonwizardscannergenerator.h"
        ]
    }

    Group {
        name: "CustomWizard"
        prefix: "customwizard/"
        files: [
            "customwizard.cpp", "customwizard.h",
            "customwizardpage.cpp", "customwizardpage.h",
            "customwizardparameters.cpp", "customwizardparameters.h",
            "customwizardpreprocessor.cpp", "customwizardpreprocessor.h",
            "customwizardscriptgenerator.cpp", "customwizardscriptgenerator.h"
        ]
    }

    Group {
        name: "Device Support"
        prefix: "devicesupport/"
        files: [
            "desktopdevice.cpp", "desktopdevice.h",
            "desktopdevicefactory.cpp", "desktopdevicefactory.h",
            "deviceapplicationrunner.cpp", "deviceapplicationrunner.h",
            "devicecheckbuildstep.cpp", "devicecheckbuildstep.h",
            "devicefactoryselectiondialog.cpp", "devicefactoryselectiondialog.h", "devicefactoryselectiondialog.ui",
            "devicemanager.cpp", "devicemanager.h",
            "devicemanagermodel.cpp", "devicemanagermodel.h",
            "deviceprocess.cpp", "deviceprocess.h",
            "deviceprocessesdialog.cpp", "deviceprocessesdialog.h",
            "deviceprocesslist.cpp", "deviceprocesslist.h",
            "devicesettingspage.cpp", "devicesettingspage.h",
            "devicesettingswidget.cpp", "devicesettingswidget.h", "devicesettingswidget.ui",
            "devicetestdialog.cpp", "devicetestdialog.h", "devicetestdialog.ui",
            "deviceusedportsgatherer.cpp", "deviceusedportsgatherer.h",
            "idevice.cpp", "idevice.h",
            "idevicefactory.cpp", "idevicefactory.h",
            "idevicewidget.h",
            "desktopdeviceprocess.cpp", "desktopdeviceprocess.h",
            "localprocesslist.cpp", "localprocesslist.h",
            "sshdeviceprocess.cpp", "sshdeviceprocess.h",
            "sshdeviceprocesslist.cpp", "sshdeviceprocesslist.h",
            "desktopprocesssignaloperation.cpp", "desktopprocesssignaloperation.h",
            "desktopdeviceconfigurationwidget.cpp", "desktopdeviceconfigurationwidget.h", "desktopdeviceconfigurationwidget.ui"
        ]
    }

    Group {
        name: "Images"
        prefix: "images/"
        files: [
            "BuildSettings.png",
            "CodeStyleSettings.png",
            "DeviceConnected.png",
            "DeviceDisconnected.png",
            "DeviceReadyToUse.png",
            "EditorSettings.png",
            "MaemoDevice.png",
            "ProjectDependencies.png",
            "RunSettings.png",
            "Simulator.png",
            "build.png",
            "build_32.png",
            "build_small.png",
            "clean.png",
            "clean_small.png",
            "closetab.png",
            "debugger_start.png",
            "debugger_start_small.png",
            "findallprojects.png",
            "findproject.png",
            "leftselection.png",
            "midselection.png",
            "projectexplorer.png",
            "rebuild.png",
            "rebuild_small.png",
            "rightselection.png",
            "run.png",
            "run_small.png",
            "session.png",
            "stop.png",
            "stop_small.png",
            "targetbuildselected.png",
            "targetleftbutton.png",
            "targetpanel_bottom.png",
            "targetpanel_gradient.png",
            "targetrightbutton.png",
            "targetrunselected.png",
            "targetseparatorbackground.png",
            "targetunselected.png",
            "window.png",
        ]
    }

    Group {
        name: "WindowsToolChains"
        condition: qbs.targetOS.contains("windows") || project.testsEnabled
        files: [
           "abstractmsvctoolchain.cpp",
           "abstractmsvctoolchain.h",
           "msvcparser.cpp",
           "msvcparser.h",
           "msvctoolchain.cpp",
           "msvctoolchain.h",
           "wincetoolchain.cpp",
           "wincetoolchain.h",
           "windebuginterface.cpp",
           "windebuginterface.h",
        ]
    }

    Group {
        name: "Tests"
        condition: project.testsEnabled
        files: ["outputparser_test.h", "outputparser_test.cpp"]
    }

    Export {
        Depends { name: "Qt.network" }
    }
}
